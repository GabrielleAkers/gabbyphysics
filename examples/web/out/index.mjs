const game_canvas = document.getElementById("game_canvas");
const ctx = game_canvas.getContext("2d");
if (!ctx)
    throw new Error("no 2d ctx");
const grid_canvas = document.getElementById("grid_canvas");
const grid_ctx = grid_canvas.getContext("2d");
if (!grid_ctx)
    throw new Error("no grid ctx");
let PARTICLE_RADIUS = 10;
const draw_grid = () => {
    const grid_step = game_canvas.height / 20;
    for (let i = 0; i <= game_canvas.height / grid_step; i += 1) {
        for (let j = 0; j <= game_canvas.width / grid_step; j += 1) {
            grid_ctx.beginPath();
            grid_ctx.strokeStyle = "blue";
            grid_ctx.setLineDash([2]);
            grid_ctx.moveTo(0, i * grid_step);
            grid_ctx.lineTo(game_canvas.width, i * grid_step);
            grid_ctx.stroke();
            grid_ctx.moveTo(j * grid_step, 0);
            grid_ctx.lineTo(j * grid_step, game_canvas.height);
            grid_ctx.stroke();
            grid_ctx.closePath();
        }
    }
};
// draw_grid();
const memory = new WebAssembly.Memory({ initial: 4 });
const import_object = {
    env: {
        memory,
        browser_draw_particles,
        browser_log,
        browser_clear_canvas
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
function browser_draw_particles(x, y, ptr) {
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
        wasm.exports.update_particles((timestamp - prev_timestamp) * 0.01);
        wasm.exports.draw_particles();
    }
    prev_timestamp = timestamp;
    window.requestAnimationFrame(loop);
}
wasm.exports.set_screen_size(game_canvas.width, game_canvas.height);
game_canvas.addEventListener("click", evt => {
    wasm.exports.spawn_particle(evt.clientX - game_canvas.offsetLeft, evt.clientY - game_canvas.offsetTop);
});
document.addEventListener("keydown", evt => {
    evt.preventDefault();
    switch (evt.code) {
        case "Space":
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
const damping_slider = document.createElement("input");
damping_slider.type = "range";
damping_slider.min = "0";
damping_slider.max = "1";
damping_slider.step = "0.001";
damping_slider.onchange = evt => {
    wasm.exports.set_damping(damping_slider.valueAsNumber);
    damping = damping_slider.valueAsNumber;
    damping_slider_label.textContent = `damping ${damping}`;
};
let damping = damping_slider.valueAsNumber;
const damping_slider_label = document.createTextNode(`damping ${damping}`);
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
particle_radius.id = "particle_radius";
particle_radius.type = "number";
particle_radius.min = "0";
particle_radius.placeholder = PARTICLE_RADIUS.toString();
particle_radius.defaultValue = PARTICLE_RADIUS.toString();
particle_radius.onchange = evt => {
    wasm.exports.set_particle_radius(particle_radius.valueAsNumber);
    PARTICLE_RADIUS = particle_radius.valueAsNumber;
};
const radius_label = document.createTextNode(`particle radius`);
container.appendChild(radius_label);
container.appendChild(particle_radius);
export {};
