interface Demo {
    name: string;
    description: string;
    path: string;
}

const available_demos: Demo[] = [
    { name: "Ball Bouncer", description: "Particle ballistics demo. Paint walls, bounce balls.", path: "./particle.mjs" },
];


(async () => {
    const main = document.getElementById("main");
    if (!main) throw new Error("No module load container 'main' found");

    let loaded_demo: { load: (load_into: HTMLElement) => void; } | null = null;

    const unload = () => {
        main.replaceChildren();
    };

    const container = document.createElement("div");
    if (!container) throw new Error("no container");
    container.style.position = "absolute";
    container.style.right = "0";
    container.style.display = "grid";
    container.style.gridTemplateColumns = "450px";
    container.style.color = "white";
    container.style.textAlign = "center";

    const title = document.createElement("p");
    title.innerText = "Examples";
    title.style.borderBottom = "1px dashed grey";
    container.appendChild(title);

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
            loaded_demo?.load(main);
        };
        row.appendChild(name);
        row.appendChild(desc);
        row.appendChild(load_btn);
        container.appendChild(row);
    });

    document.body.insertAdjacentElement("afterbegin", container);
})();
