const canvas = document.createElement("canvas");
const ctx = canvas.getContext("2d");
if (!ctx) throw new Error("no compass ctx");
let compass_size = 80;

const offscreen = document.createElement("canvas");
const offscreen_ctx = offscreen.getContext("2d");
if (!offscreen_ctx) throw new Error("no compass offscreen ctx");

function set_compass_size(r: number) {
    compass_size = r;
    canvas.width = 2 * compass_size;
    canvas.height = 2 * compass_size;
    offscreen.width = canvas.width;
    offscreen.height = canvas.height;
}

const compass_center = () => ({
    x: compass_size,
    y: compass_size
});

const draw_offscreen_compass = () => {
    const center = compass_center();
    offscreen_ctx.beginPath();
    offscreen_ctx.arc(center.x, center.y, 5, 0, 2 * Math.PI);
    offscreen_ctx.strokeStyle = "red";
    offscreen_ctx.fillStyle = "red";
    offscreen_ctx.stroke();
    offscreen_ctx.fill();
    offscreen_ctx.closePath();
    offscreen_ctx.beginPath();
    offscreen_ctx.arc(center.x, center.y, compass_size, 0, 2 * Math.PI);
    offscreen_ctx.strokeStyle = "white";
    offscreen_ctx.stroke();
    offscreen_ctx.closePath();
};

const draw_compass = () => {
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    ctx.drawImage(offscreen, 0, 0);
};

interface Vec2 {
    x: number;
    y: number;
}

export const scale_to_len = (v: Vec2, len: number) => {
    const l = Math.sqrt(v.x * v.x + v.y * v.y);
    return {
        x: len * v.x / l,
        y: len * v.y / l
    };
};

const draw_line = (to: Vec2) => {
    const center = compass_center();
    const to_edge = scale_to_len({ x: to.x - center.x, y: to.y - center.y }, compass_size);
    ctx.beginPath();
    ctx.strokeStyle = "red";
    ctx.arc(center.x + to_edge.x, center.y + to_edge.y, 5, 0, 2 * Math.PI, true);
    ctx.stroke();
    ctx.fill();
    ctx.closePath();

    ctx.beginPath();
    ctx.moveTo(center.x, center.y);
    ctx.lineTo(center.x + to_edge.x, center.y + to_edge.y);
    ctx.lineCap = "square";
    ctx.strokeStyle = "red";
    ctx.lineWidth = 3;
    ctx.stroke();
    ctx.closePath();
};

export const Compass = (on_direction_change: (new_direction: Vec2) => void, size: number = 80) => {
    set_compass_size(size);
    draw_offscreen_compass();
    draw_compass();
    // set default to 'south'
    const c = compass_center();
    draw_line({ x: c.x, y: c.y + 1 });
    on_direction_change({ x: 0, y: 1 });

    canvas.addEventListener("click", evt => {
        const center = compass_center();
        const to = { x: evt.clientX - canvas.offsetLeft, y: evt.clientY - canvas.offsetTop };
        draw_compass();
        draw_line(to);
        on_direction_change(scale_to_len({ x: to.x - center.x, y: to.y - center.y }, 1));
    });
    return canvas;
};
