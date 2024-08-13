import { Compass, scale_to_len } from "./components/compass_input.mjs";
import { ImageButton } from "./components/image_button.mjs";
import { ElementLoaderCallback } from "./loader.mjs";
import { cstr_by_ptr } from "./util.mjs";

export const load = async (load_elems: ElementLoaderCallback, update_timer: (n: number) => void, reset_timer: () => void) => {
    const game_canvas = document.createElement("canvas");
    game_canvas.id = "game_canvas";
    game_canvas.width = 800;
    game_canvas.height = 800;
    const ctx = game_canvas.getContext("2d");
    if (!ctx) throw new Error("no 2d ctx");

    const DEFAULT_SIM_SPEED = 0.01;
    const DEFAULT_PARTICLE_RADIUS = 5;
    const DEFAULT_DAMPING = 0.95;
    const DEFAULT_GRAVITY_SCALE = 9.81;
    let SIM_SPEED = 0.01;
    let DAMPING = 0.95;
    let PARTICLE_RADIUS = 5;
    let GRAVITY_SCALE = 9.81;

    const memory = new WebAssembly.Memory({ initial: 4 });
    const import_object = {
        env: {
            memory,
            browser_draw_point,
            browser_log,
            browser_clear_canvas,
            browser_draw_rect: (...args: any[]) => { },
            browser_draw_line,
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
            init: (x: number, y: number) => void;
            create_cable: (x1: number, y1: number, x2: number, y2: number) => void;
            main: () => number;
            // reset_particles: () => void;
            // set_damping: (d: number) => void;
            // set_particle_radius: (r: number) => void;
            // set_gravity: (x: number, y: number) => void;
            // init_grid: () => void;
            // paint_wall: (x: number, y: number) => void;
        };
    }
    const wasm = (await WebAssembly.instantiateStreaming(fetch("out/bridgesim.wasm"), import_object)).instance as WasmInstance;

    function browser_draw_point(x: number, y: number, size: number, r: number, g: number, b: number) {
        if (!ctx) return;

        ctx.beginPath();
        ctx.arc(x, y, size, 0, 2 * Math.PI);
        ctx.fillStyle = `rgb(${r}, ${g}, ${b})`;
        ctx.fill();
        ctx.closePath();
    };

    function browser_draw_line(x1: number, y1: number, x2: number, y2: number, r: number, g: number, b: number) {
        if (!ctx) return;

        ctx.beginPath();
        ctx.moveTo(x1, y1);
        ctx.lineTo(x2, y2);
        ctx.strokeStyle = `rgb(${r}, ${g}, ${b})`;
        ctx.stroke();
        ctx.closePath();
    }

    function browser_log(log_ptr: number) {
        const buffer = memory.buffer;
        const message = cstr_by_ptr(buffer, log_ptr);
        console.log(message);
    }

    function browser_clear_canvas() {
        if (!ctx) return;

        ctx.clearRect(0, 0, game_canvas.width, game_canvas.height);
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
        frame_ids.push(window.requestAnimationFrame(loop));
    }

    // wasm.exports.init(game_canvas.width, game_canvas.height);
    wasm.exports.main();

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


    type ClickMode = "spawn_particle" | "create_component" | null;
    let click_mode: ClickMode;
    let click_buffer: number[][] = []; // [[x1, y1], [x2, y2]]
    type ComponentTypes = "CABLE" | "ROD" | "FIXED_CABLE" | "FIXED_ROD" | "PARTICLE";
    let current_cmd: ComponentTypes | null = null;

    const component_cmds: { [K in ComponentTypes]: Function; } = {
        CABLE: () => {
            console.log("create cable at", click_buffer[0], click_buffer[1]);
            wasm.exports.create_cable(click_buffer[0][0], click_buffer[0][1], click_buffer[1][0], click_buffer[1][1]);
        },
        ROD: () => console.log("rod"),
        FIXED_CABLE: () => console.log("fixed_cable"),
        FIXED_ROD: () => console.log("fixed_rod"),
        PARTICLE: () => wasm.exports.spawn_particle(click_buffer[0][0], click_buffer[0][1])
    };

    const component_command_queue: Array<ComponentTypes> = [];

    const exec_command = (cmd: ComponentTypes) => {
        if (Object.hasOwn(component_cmds, cmd)) {
            component_cmds[cmd]();
            component_command_queue.push(cmd);
        }
    };

    const undo_command = () => {
        if (component_command_queue.length > 0) {
            component_command_queue.pop();
        }
    };

    game_canvas.addEventListener("click", evt => {
        switch (click_mode) {
            case "create_component":
                click_buffer.push([evt.clientX - game_canvas.offsetLeft, evt.clientY - game_canvas.offsetTop]);
                if (click_buffer.length == 2) {
                    if (current_cmd) {
                        exec_command(current_cmd);
                    }
                    click_buffer = [];
                }
                break;
            case "spawn_particle":
                click_buffer.push([evt.clientX - game_canvas.offsetLeft, evt.clientY - game_canvas.offsetTop]);
                if (click_buffer.length == 1) {
                    if (current_cmd) {
                        exec_command(current_cmd);
                    }
                    click_buffer = [];
                }
                break;
            default:
                break;
        }
    });

    click_mode = null;

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

    const spring_cmds_label = document.createTextNode("Spawnable");
    container.appendChild(spring_cmds_label);
    const img_btn_holder = document.createElement("div");
    img_btn_holder.style.display = "grid";
    img_btn_holder.style.gridTemplateColumns = "repeat(2, 80px)";
    img_btn_holder.style.textAlign = "center";
    container.appendChild(img_btn_holder);

    const create_component = (cmd: ComponentTypes) => {
        console.log("create mode: ", cmd);
        current_cmd = cmd;
        if (cmd === "PARTICLE")
            click_mode = "spawn_particle";
        else
            click_mode = "create_component";
    };
    const cable_btn = ImageButton("../assets/cable.png", 80, 80, () => create_component("CABLE"), "Cable");
    const rod_btn = ImageButton("../assets/rod.png", 80, 80, () => create_component("ROD"), "Rod");
    const fixed_cable_btn = ImageButton("../assets/fixed_cable.png", 80, 80, () => create_component("FIXED_CABLE"), "Fixed Cable");
    const fixed_rod_btn = ImageButton("../assets/fixed_rod.png", 80, 80, () => create_component("FIXED_ROD"), "Fixed Rod");
    const particle_btn = ImageButton("../assets/particle.png", 80, 80, () => create_component("PARTICLE"), "Particle");

    img_btn_holder.appendChild(cable_btn);
    img_btn_holder.appendChild(rod_btn);
    img_btn_holder.appendChild(fixed_cable_btn);
    img_btn_holder.appendChild(fixed_rod_btn);
    container.appendChild(particle_btn);

    // const sim_speed_slider = document.createElement("input");
    // sim_speed_slider.type = "range";
    // sim_speed_slider.min = "0.001";
    // sim_speed_slider.max = "0.1";
    // sim_speed_slider.step = "0.0001";
    // sim_speed_slider.defaultValue = DEFAULT_SIM_SPEED.toString();
    // SIM_SPEED = sim_speed_slider.valueAsNumber;
    // const sim_speed_slider_label = document.createTextNode(`sim speed ${SIM_SPEED}`);
    // sim_speed_slider.onchange = (evt) => {
    //     // @ts-ignore
    //     SIM_SPEED = evt.target.value;
    //     sim_speed_slider_label.textContent = `sim speed ${SIM_SPEED}`;
    // };

    // container.appendChild(sim_speed_slider_label);
    // container.appendChild(sim_speed_slider);

    // const damping_slider = document.createElement("input");
    // damping_slider.type = "range";
    // damping_slider.min = "0";
    // damping_slider.max = "1";
    // damping_slider.step = "0.001";
    // damping_slider.defaultValue = DEFAULT_DAMPING.toString();
    // DAMPING = damping_slider.valueAsNumber;
    // const damping_slider_label = document.createTextNode(`damping ${DAMPING}`);
    // damping_slider.onchange = evt => {
    //     // @ts-ignore
    //     DAMPING = evt.target.value;;
    //     wasm.exports.set_damping(DAMPING);

    //     damping_slider_label.textContent = `damping ${DAMPING}`;
    // };

    // container.appendChild(damping_slider_label);
    // container.appendChild(damping_slider);

    // const reset_btn = document.createElement("button");
    // reset_btn.innerText = "Reset Particles";
    // reset_btn.onclick = evt => {
    //     evt.preventDefault();
    //     wasm.exports.reset_particles();
    // };
    // container.appendChild(reset_btn);

    // const particle_radius = document.createElement("input");
    // particle_radius.type = "number";
    // particle_radius.min = "0";
    // particle_radius.placeholder = DEFAULT_PARTICLE_RADIUS.toString();
    // particle_radius.defaultValue = DEFAULT_PARTICLE_RADIUS.toString();
    // particle_radius.onchange = evt => {
    //     // @ts-ignore
    //     PARTICLE_RADIUS = evt.target.value;
    //     wasm.exports.set_particle_radius(PARTICLE_RADIUS);
    // };
    // const radius_label = document.createTextNode(`particle radius`);
    // container.appendChild(radius_label);
    // container.appendChild(particle_radius);

    // const gravity_scale = document.createElement("input");
    // gravity_scale.type = "number";
    // gravity_scale.min = "-20";
    // gravity_scale.max = "20";
    // gravity_scale.placeholder = DEFAULT_GRAVITY_SCALE.toString();
    // gravity_scale.defaultValue = DEFAULT_GRAVITY_SCALE.toString();
    // gravity_scale.onchange = evt => {
    //     // @ts-ignore
    //     GRAVITY_SCALE = evt.target.value;
    // };
    // const gravity_label = document.createTextNode(`gravity`);
    // container.appendChild(gravity_label);
    // container.appendChild(gravity_scale);

    // function gravity_dir_handler(dir: { x: number, y: number; }) {
    //     const gravity = scale_to_len(dir, GRAVITY_SCALE);
    //     wasm.exports.set_gravity(gravity.x, gravity.y);
    // };
    // container.appendChild(Compass(gravity_dir_handler));

    load_elems([game_canvas, container]);

    return () => {
        frame_ids.forEach(id => window.cancelAnimationFrame(id));
        frame_ids = [];
        prev_timestamp = null;
        started = false;
        reset_timer();
    };
};
