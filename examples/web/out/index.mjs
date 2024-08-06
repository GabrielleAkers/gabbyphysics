import { Compass, scale_to_len } from "./compass_input.mjs";
const game_canvas = document.getElementById("game_canvas");
const ctx = game_canvas.getContext("2d");
if (!ctx)
    throw new Error("no 2d ctx");
const grid_canvas = document.getElementById("grid_canvas");
const grid_ctx = grid_canvas.getContext("2d");
if (!grid_ctx)
    throw new Error("no grid ctx");
const offscreen_grid = document.createElement("canvas");
offscreen_grid.width = grid_canvas.width;
offscreen_grid.height = grid_canvas.height;
const offscreen_grid_ctx = offscreen_grid.getContext("2d");
if (!offscreen_grid_ctx)
    throw new Error("no offscreen grid ctx");
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
        browser_draw_particles,
        browser_log,
        browser_clear_canvas,
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
        }
    }
};
const wasm = (await WebAssembly.instantiateStreaming(fetch("out/main.wasm"), import_object)).instance;
function browser_draw_particles(x, y) {
    if (!ctx)
        return;
    ctx.beginPath();
    ctx.arc(x, y, PARTICLE_RADIUS, 0, 2 * Math.PI);
    ctx.fillStyle = "blue";
    ctx.fill();
    ctx.closePath();
}
;
function cstrlen(buff, ptr) {
    let len = 0;
    while (buff[ptr] !== 0) {
        len++;
        ptr++;
    }
    return len;
}
function cstr_by_ptr(buff, ptr) {
    const mem = new Uint8Array(buff);
    const len = cstrlen(mem, ptr);
    const bytes = new Uint8Array(buff, ptr, len);
    return new TextDecoder().decode(bytes);
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
    // draw_grid();
}
let prev_timestamp = null;
let started = false;
function loop(timestamp) {
    if (!started) {
        prev_timestamp = null;
        return;
    }
    ;
    if (prev_timestamp !== null) {
        wasm.exports.update_particles((timestamp - prev_timestamp) * SIM_SPEED);
        wasm.exports.draw_particles();
    }
    prev_timestamp = timestamp;
    window.requestAnimationFrame(loop);
}
wasm.exports.set_screen_width(game_canvas.width);
game_canvas.addEventListener("click", evt => {
    wasm.exports.spawn_particle(evt.clientX - game_canvas.offsetLeft, evt.clientY - game_canvas.offsetTop);
});
document.addEventListener("keydown", evt => {
    switch (evt.code) {
        case "Space":
            evt.preventDefault();
            started = !started;
            window.requestAnimationFrame(loop);
            break;
    }
});
const container = document.getElementById("container");
container.style.display = "grid";
container.style.gridTemplateColumns = "160px";
container.style.color = "white";
container.style.textAlign = "center";
const start_button = document.createElement("button");
start_button.innerText = "Toggle Simulation (spacebar)";
start_button.onclick = (evt) => {
    evt.preventDefault();
    started = !started;
    window.requestAnimationFrame(loop);
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
    DAMPING = evt.target.value;
    ;
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
const gravity_dir_handler = (dir) => {
    console.log(dir);
    const gravity = scale_to_len(dir, GRAVITY_SCALE);
    wasm.exports.set_gravity(gravity.x, gravity.y);
};
container.appendChild(Compass(gravity_dir_handler));
