import { Compass, scale_to_len } from "./components/compass_input.mjs";
import { cstr_by_ptr } from "./util.mjs";
export const load = async (load_elems, update_timer, reset_timer) => {
    const game_canvas = document.createElement("canvas");
    game_canvas.id = "game_canvas";
    game_canvas.width = 800;
    game_canvas.height = 800;
    const ctx = game_canvas.getContext("2d");
    if (!ctx)
        throw new Error("no 2d ctx");
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
            browser_draw_rect: (...args) => { },
            browser_draw_line,
            browser_draw_radial_gradient
        },
        // wasi-sdk adds this import namespace when compiling to wasm32-wasi which is default unless --target=wasm32
        wasi_snapshot_preview1: {
            fd_close(...args) {
                return 0;
            },
            fd_seek(...args) {
                return 0;
            },
            fd_write(...args) {
                return 0;
            },
            random_get: Math.random
        }
    };
    const wasm = (await WebAssembly.instantiateStreaming(fetch("out/watersim.wasm"), import_object)).instance;
    wasm.exports.main();
    function browser_draw_point(x, y, size, r, g, b) {
        if (!ctx)
            return;
        ctx.beginPath();
        ctx.arc(x, y, size, 0, 2 * Math.PI);
        ctx.fillStyle = `rgb(${r}, ${g}, ${b})`;
        ctx.fill();
        ctx.closePath();
    }
    ;
    function browser_draw_radial_gradient(x, y, inner_r, outer_r, r1, g1, b1, r2, g2, b2) {
        if (!ctx)
            return;
        ctx.beginPath();
        const gradient = ctx.createRadialGradient(x, y, inner_r, x, y, outer_r);
        gradient.addColorStop(0, `rgb(${r1}, ${g1}, ${b1})`);
        gradient.addColorStop(0, `rgb(${r2}, ${g2}, ${b2})`);
        ctx.arc(x, y, outer_r, 0, 2 * Math.PI);
        ctx.fillStyle = gradient;
        ctx.fill();
        ctx.closePath();
    }
    function browser_draw_line(x1, y1, x2, y2, r, g, b) {
        if (!ctx)
            return;
        ctx.beginPath();
        ctx.moveTo(x1, y1);
        ctx.lineTo(x2, y2);
        ctx.strokeStyle = `rgb(${r}, ${g}, ${b})`;
        ctx.stroke();
        ctx.closePath();
    }
    function browser_log(log_ptr) {
        const buffer = memory.buffer;
        const message = cstr_by_ptr(buffer, log_ptr);
        console.log(message);
    }
    function browser_clear_canvas() {
        if (!ctx)
            return;
        ctx.clearRect(0, 0, game_canvas.width, game_canvas.height);
    }
    let prev_timestamp = null;
    let started = false;
    let frame_ids = [];
    function loop(timestamp) {
        if (!started) {
            prev_timestamp = null;
            return;
        }
        ;
        if (prev_timestamp !== null) {
            wasm.exports.update_particles((timestamp - prev_timestamp) * SIM_SPEED);
            wasm.exports.draw_particles();
            if (Math.floor(((timestamp - prev_timestamp) / 1000) % 60) === 0)
                update_timer(timestamp - prev_timestamp);
        }
        prev_timestamp = timestamp;
        frame_ids.push(window.requestAnimationFrame(loop));
    }
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
        DAMPING = evt.target.value;
        ;
        wasm.exports.set_damping(DAMPING);
        damping_slider_label.textContent = `damping ${DAMPING}`;
    };
    container.appendChild(damping_slider_label);
    container.appendChild(damping_slider);
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
    function gravity_dir_handler(dir) {
        const gravity = scale_to_len(dir, GRAVITY_SCALE);
        wasm.exports.set_gravity(gravity.x, gravity.y);
    }
    ;
    container.appendChild(Compass(gravity_dir_handler));
    load_elems([game_canvas, container]);
    return () => {
        frame_ids.forEach(id => window.cancelAnimationFrame(id));
        frame_ids = [];
        prev_timestamp = null;
        started = false;
        reset_timer();
    };
};
