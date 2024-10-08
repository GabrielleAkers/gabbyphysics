import { Compass, scale_to_len } from "./components/compass_input.mjs";
import { ElementLoaderCallback } from "./loader.mjs";
import { cstr_by_ptr } from "./util.mjs";

export const load = async (load_elems: ElementLoaderCallback, update_timer: (n: number) => void, reset_timer: () => void) => {
    const game_canvas = document.createElement("canvas");
    game_canvas.id = "game_canvas";
    game_canvas.width = 800;
    game_canvas.height = 800;
    const ctx = game_canvas.getContext("2d");
    if (!ctx) throw new Error("no 2d ctx");

    const grid_canvas = document.createElement("canvas");
    grid_canvas.id = "grid_canvas";
    grid_canvas.width = 800;
    grid_canvas.height = 800;
    const grid_ctx = grid_canvas.getContext("2d");
    if (!grid_ctx) throw new Error("no grid ctx");

    const offscreen_grid = document.createElement("canvas");
    offscreen_grid.width = grid_canvas.width;
    offscreen_grid.height = grid_canvas.height;
    const offscreen_grid_ctx = offscreen_grid.getContext("2d");
    if (!offscreen_grid_ctx) throw new Error("no offscreen grid ctx");

    const offscreen_grid_lines = document.createElement("canvas");
    offscreen_grid_lines.width = grid_canvas.width;
    offscreen_grid_lines.height = grid_canvas.height;
    const offscreen_grid_lines_ctx = offscreen_grid_lines.getContext("2d");
    if (!offscreen_grid_lines_ctx) throw new Error("no offscreen grid ctx");

    const DEFAULT_SIM_SPEED = 0.01;
    const DEFAULT_PARTICLE_RADIUS = 5;
    const DEFAULT_DAMPING = 0.95;
    const DEFAULT_GRAVITY_SCALE = 9.81;
    let SIM_SPEED = 0.01;
    let DAMPING = 0.95;
    let PARTICLE_RADIUS = 5;
    let GRAVITY_SCALE = 9.81;
    let DRAW_GRIDLINES = false;

    const memory = new WebAssembly.Memory({ initial: 4 });
    const import_object = {
        env: {
            memory,
            browser_draw_point,
            browser_log,
            browser_clear_canvas,
            browser_draw_rect,
            browser_draw_radial_gradient: () => { }
        },
        // wasi-sdk adds this import namespace when compiling to wasm32-wasi which is default unless --target=wasm32
        wasi_snapshot_preview1: {
            fd_close(...args: any[]): any {
                return 0;
            },
            fd_seek(...args: any[]): any {
                return 0;
            },
            fd_write(...args: any[]): any {
                return 0;
            },
            random_get: Math.random
        }
    };

    interface WasmInstance extends WebAssembly.Instance {
        exports: {
            spawn_particle: (x: number, y: number) => void;
            update_particles: (delta_time: number) => void;
            draw_particles: () => void;
            reset_particles: () => void;
            set_screen_size: (x: number, y: number) => void;
            set_damping: (d: number) => void;
            set_particle_radius: (r: number) => void;
            set_gravity: (x: number, y: number) => void;
            init_grid: () => void;
            paint_wall: (x: number, y: number) => void;
        };
    }
    const wasm = (await WebAssembly.instantiateStreaming(fetch("out/particlesim.wasm"), import_object)).instance as WasmInstance;

    function browser_draw_point(x: number, y: number, size: number, r: number, g: number, b: number) {
        if (!ctx) return;

        ctx.beginPath();
        ctx.arc(x, y, size, 0, 2 * Math.PI);
        ctx.fillStyle = `rgb(${r}, ${g}, ${b})`;
        ctx.fill();
        ctx.closePath();
    };

    const CellTypeColorMap: { [type: number]: string; } = {
        0: "rgba(0, 1, 200, 0.2)",
        1: "rgba(0, 0, 0, 0)",
        2: "rgba(200, 200, 200, 1)",
    };

    function browser_draw_rect(x: number, y: number, type: number, cell_w: number, cell_h: number) {
        let color: string = "rgb(227, 61, 148)";
        if (Object.hasOwn(CellTypeColorMap, type))
            color = CellTypeColorMap[type];
        offscreen_grid_ctx!.fillStyle = color;
        offscreen_grid_ctx!.fillRect(x, y, cell_w, cell_h);
        grid_ctx!.drawImage(offscreen_grid, 0, 0);

        offscreen_grid_lines_ctx!.strokeStyle = "grey";
        offscreen_grid_lines_ctx!.strokeRect(x, y, cell_w, cell_h);
        if (DRAW_GRIDLINES) {
            grid_ctx!.drawImage(offscreen_grid_lines, 0, 0);
        }
    }

    function browser_log(log_ptr: number) {
        const buffer = memory.buffer;
        const message = cstr_by_ptr(buffer, log_ptr);
        console.log(message);
    }

    function browser_clear_canvas() {
        if (!ctx) return;

        ctx.clearRect(0, 0, game_canvas.width, game_canvas.height);
        // draw_grid();
    }

    let prev_timestamp: number | null = null;
    let started = false;
    let frame_ids: number[] = [];
    function loop(timestamp: number) {
        if (!started) {
            prev_timestamp = null;
            return;
        };

        if (prev_timestamp !== null) {
            wasm.exports.update_particles((timestamp - prev_timestamp) * SIM_SPEED);
            wasm.exports.draw_particles();
            if (Math.floor(((timestamp - prev_timestamp) / 1000) % 60) === 0)
                update_timer(timestamp - prev_timestamp);
        }
        prev_timestamp = timestamp;

        frame_ids.forEach(id => window.cancelAnimationFrame(id));
        frame_ids.push(window.requestAnimationFrame(loop));
    }

    wasm.exports.set_screen_size(game_canvas.width, game_canvas.height);
    wasm.exports.init_grid();

    type ClickMode = "spawn" | "paint";
    let click_mode: ClickMode;

    game_canvas.addEventListener("click", evt => {
        switch (click_mode) {
            case "spawn":
                wasm.exports.spawn_particle(evt.clientX - game_canvas.offsetLeft, evt.clientY - game_canvas.offsetTop);
                break;
            case "paint":
                wasm.exports.paint_wall(evt.clientX - game_canvas.offsetLeft, evt.clientY - game_canvas.offsetTop);
                break;

        }
    });

    click_mode = "spawn";

    document.addEventListener("keydown", evt => {
        switch (evt.code) {
            case "Space":
                evt.preventDefault();
                frame_ids.forEach(id => window.cancelAnimationFrame(id));
                started = !started;
                frame_ids.push(window.requestAnimationFrame(loop));
                break;
        }
    });

    const container = document.createElement("div");
    container.style.display = "grid";
    container.style.gridTemplateColumns = "160px";
    container.style.color = "white";
    container.style.textAlign = "center";

    const start_button = document.createElement("button");
    start_button.innerText = "Toggle Simulation (spacebar)";
    start_button.onclick = (evt) => {
        evt.preventDefault();
        frame_ids.forEach(id => window.cancelAnimationFrame(id));
        started = !started;
        frame_ids.push(window.requestAnimationFrame(loop));
    };
    container.appendChild(start_button);

    const sim_speed_slider = document.createElement("input");
    sim_speed_slider.type = "range";
    sim_speed_slider.min = "0.001";
    sim_speed_slider.max = "0.1";
    sim_speed_slider.step = "0.0001";
    sim_speed_slider.defaultValue = DEFAULT_SIM_SPEED.toString();
    SIM_SPEED = sim_speed_slider.valueAsNumber;
    const sim_speed_slider_label = document.createTextNode(`sim speed ${SIM_SPEED}`);
    sim_speed_slider.onchange = (evt) => {
        // @ts-ignore
        SIM_SPEED = evt.target.value;
        sim_speed_slider_label.textContent = `sim speed ${SIM_SPEED}`;
    };

    container.appendChild(sim_speed_slider_label);
    container.appendChild(sim_speed_slider);

    const damping_slider = document.createElement("input");
    damping_slider.type = "range";
    damping_slider.min = "0";
    damping_slider.max = "1";
    damping_slider.step = "0.001";
    damping_slider.defaultValue = DEFAULT_DAMPING.toString();
    DAMPING = damping_slider.valueAsNumber;
    const damping_slider_label = document.createTextNode(`damping ${DAMPING}`);
    damping_slider.onchange = evt => {
        // @ts-ignore
        DAMPING = evt.target.value;;
        wasm.exports.set_damping(DAMPING);

        damping_slider_label.textContent = `damping ${DAMPING}`;
    };

    container.appendChild(damping_slider_label);
    container.appendChild(damping_slider);

    const reset_btn = document.createElement("button");
    reset_btn.innerText = "Reset Particles";
    reset_btn.onclick = evt => {
        evt.preventDefault();
        wasm.exports.reset_particles();
    };
    container.appendChild(reset_btn);

    const particle_radius = document.createElement("input");
    particle_radius.type = "number";
    particle_radius.min = "0";
    particle_radius.placeholder = DEFAULT_PARTICLE_RADIUS.toString();
    particle_radius.defaultValue = DEFAULT_PARTICLE_RADIUS.toString();
    particle_radius.onchange = evt => {
        // @ts-ignore
        PARTICLE_RADIUS = evt.target.value;
        wasm.exports.set_particle_radius(PARTICLE_RADIUS);
    };
    const radius_label = document.createTextNode(`particle radius`);
    container.appendChild(radius_label);
    container.appendChild(particle_radius);

    const gravity_scale = document.createElement("input");
    gravity_scale.type = "number";
    gravity_scale.min = "-20";
    gravity_scale.max = "20";
    gravity_scale.placeholder = DEFAULT_GRAVITY_SCALE.toString();
    gravity_scale.defaultValue = DEFAULT_GRAVITY_SCALE.toString();
    gravity_scale.onchange = evt => {
        // @ts-ignore
        GRAVITY_SCALE = evt.target.value;
    };
    const gravity_label = document.createTextNode(`gravity`);
    container.appendChild(gravity_label);
    container.appendChild(gravity_scale);

    function gravity_dir_handler(dir: { x: number, y: number; }) {
        const gravity = scale_to_len(dir, GRAVITY_SCALE);
        wasm.exports.set_gravity(gravity.x, gravity.y);
    };
    container.appendChild(Compass(gravity_dir_handler));

    const click_mode_label = document.createTextNode(`click mode: ${click_mode}`);
    container.appendChild(click_mode_label);
    const btn_holder = document.createElement("div");
    btn_holder.style.display = "grid";
    btn_holder.style.gridTemplateColumns = "repeat(2, 80px)";
    const spawn_btn = document.createElement("button");
    spawn_btn.innerText = "Spawn";
    const paint_btn = document.createElement("button");
    paint_btn.innerText = "Paint";
    const highlight_btn = document.createElement("button");
    highlight_btn.innerText = "Color";
    spawn_btn.onclick = () => {
        click_mode = "spawn";
        click_mode_label.textContent = `click mode: ${click_mode}`;
    };
    paint_btn.onclick = () => {
        click_mode = "paint";
        click_mode_label.textContent = `click mode: ${click_mode}`;
    };

    btn_holder.appendChild(spawn_btn);
    btn_holder.appendChild(paint_btn);
    container.appendChild(btn_holder);

    const toggle_gridlines_btn = document.createElement("button");
    toggle_gridlines_btn.innerText = "Toggle Gridlines";
    toggle_gridlines_btn.onclick = () => {
        DRAW_GRIDLINES = !DRAW_GRIDLINES;
        if (DRAW_GRIDLINES) {
            grid_ctx?.drawImage(offscreen_grid, 0, 0);
            grid_ctx?.drawImage(offscreen_grid_lines, 0, 0);
        } else {
            grid_ctx?.clearRect(0, 0, grid_canvas.width, grid_canvas.height);
            grid_ctx?.drawImage(offscreen_grid, 0, 0);
        }
    };
    container.appendChild(toggle_gridlines_btn);

    load_elems([grid_canvas, game_canvas, container]);

    return () => {
        frame_ids.forEach(id => window.cancelAnimationFrame(id));
        frame_ids = [];
        prev_timestamp = null;
        started = false;
        reset_timer();
    };
};
