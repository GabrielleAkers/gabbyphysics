import { SevenSegmentDisplay } from "./components/seven_segment_display.mjs";
const available_demos = [
    { name: "Ball Bouncer", description: "Particle ballistics demo. Paint walls, bounce balls.", path: "./particle.mjs" },
    // { name: "Water Sim", description: "It simulates water.", path: "./watersim.mjs" },
    // { name: "Bridges and Ropes", description: "Create objects out of springlike things", path: "./bridge.mjs" }
];
const LAST_DEMO_STORAGE_KEY = "LAST_DEMO";
(async () => {
    const main = document.getElementById("main");
    if (!main)
        throw new Error("No module load container 'main' found");
    let loaded_demo = null;
    let loaded_demo_cleanup;
    const unload = () => {
        if (loaded_demo_cleanup)
            loaded_demo_cleanup();
        main.replaceChildren();
    };
    const load_elems = (els) => {
        els.forEach(el => {
            try {
                main.appendChild(el);
            }
            catch (err) {
                console.error(`Couldnt load element ${el}. Error:`, err);
            }
        });
    };
    const parse_hms = (n) => {
        const s = Math.floor((n / 1000) % 60);
        const m = Math.floor((n / (1000 * 60)) % 60);
        const h = Math.floor((n / (1000 * 60 * 60)) % 24);
        return [h, m, s];
    };
    let time = 0;
    let timer_val = [0, 0, 0]; // [h, m, s]
    const [Hours, set_hours] = SevenSegmentDisplay(2, 30, 60, timer_val[0], "h");
    const [Minutes, set_minutes] = SevenSegmentDisplay(2, 30, 60, timer_val[1], "m");
    const [Seconds, set_seconds] = SevenSegmentDisplay(2, 30, 60, timer_val[2], "s");
    const update_timer = (n) => {
        time += n;
        let new_val = parse_hms(time);
        if (timer_val[0] !== new_val[0]) {
            timer_val[0] = new_val[0];
            set_hours(timer_val[0]);
        }
        if (timer_val[1] !== new_val[1]) {
            timer_val[1] = new_val[1];
            set_minutes(timer_val[1]);
        }
        if (timer_val[2] !== new_val[2]) {
            timer_val[2] = new_val[2];
            set_seconds(timer_val[2]);
        }
    };
    const reset_timer = () => {
        time = 0;
        timer_val = [0, 0, 0];
        set_hours(0);
        set_minutes(0);
        set_seconds(0);
    };
    const last_demo_name = window.localStorage.getItem(LAST_DEMO_STORAGE_KEY);
    if (last_demo_name) {
        const demo = available_demos.find(d => d.name == last_demo_name);
        if (demo) {
            loaded_demo = await import(demo.path);
            loaded_demo_cleanup = await loaded_demo?.load(load_elems, update_timer, reset_timer);
            const title = document.getElementById("title");
            if (title)
                title.innerText = demo.name;
        }
        else {
            window.localStorage.setItem(LAST_DEMO_STORAGE_KEY, "");
        }
    }
    const footer = document.getElementById("footer");
    if (footer) {
        const seven_seg_timer = document.createElement("div");
        seven_seg_timer.style.display = "flex";
        seven_seg_timer.style.alignItems = "center";
        seven_seg_timer.style.justifyContent = "center";
        seven_seg_timer.appendChild(Hours);
        seven_seg_timer.appendChild(Minutes);
        seven_seg_timer.appendChild(Seconds);
        footer.appendChild(seven_seg_timer);
    }
    const examples_container = document.createElement("div");
    if (!examples_container)
        throw new Error("no container");
    examples_container.style.position = "absolute";
    examples_container.style.right = "0";
    examples_container.style.display = "grid";
    examples_container.style.gridTemplateColumns = "450px";
    examples_container.style.color = "white";
    examples_container.style.textAlign = "center";
    const title = document.createElement("p");
    title.innerText = "Examples";
    title.style.borderBottom = "1px dashed grey";
    examples_container.appendChild(title);
    available_demos.forEach(demo => {
        const row = document.createElement("div");
        row.style.margin = "auto";
        row.style.textAlign = "center";
        row.style.display = "grid";
        row.style.gridTemplateColumns = "repeat(3, 150px)";
        row.style.border = "1px dashed grey";
        const name = document.createElement("p");
        name.innerText = demo.name;
        name.style.margin = "auto";
        const desc = document.createElement("p");
        desc.innerText = demo.description;
        desc.style.margin = "auto";
        const load_btn = document.createElement("button");
        load_btn.innerText = "Load";
        load_btn.onclick = async (evt) => {
            evt.preventDefault();
            if (loaded_demo)
                unload();
            loaded_demo = await import(demo.path);
            if (loaded_demo) {
                loaded_demo_cleanup = await loaded_demo.load(load_elems, update_timer, reset_timer);
                window.localStorage.setItem(LAST_DEMO_STORAGE_KEY, demo.name);
                const title = document.getElementById("title");
                if (title)
                    title.innerText = demo.name;
            }
            else {
                window.localStorage.setItem(LAST_DEMO_STORAGE_KEY, "");
            }
        };
        row.appendChild(name);
        row.appendChild(desc);
        row.appendChild(load_btn);
        examples_container.appendChild(row);
    });
    document.body.insertAdjacentElement("afterbegin", examples_container);
})();
