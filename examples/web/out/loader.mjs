const available_demos = [
    { name: "particle", description: "Particle ballistics demo. Paint walls, bounce balls.", path: "./particle.mjs" }
];
(async () => {
    let loaded_demo = null;
    if (!loaded_demo) {
        const container = document.createElement("div");
        if (!container)
            throw new Error("no container");
        container.style.position = "absolute";
        container.style.right = "0";
        container.style.display = "grid";
        container.style.gridTemplateColumns = "450px";
        container.style.color = "white";
        container.style.textAlign = "center";
        const header = document.createElement("div");
        header.style.textAlign = "center";
        header.style.display = "grid";
        header.style.gridTemplateColumns = "repeat(3, 150px)";
        const h_name = document.createElement("p");
        h_name.innerText = "Name";
        h_name.style.margin = "auto";
        const h_desc = document.createElement("p");
        h_desc.innerText = "Description";
        h_desc.style.margin = "auto";
        header.appendChild(h_name);
        header.appendChild(h_desc);
        container.appendChild(header);
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
            const load_btn = document.createElement("button");
            load_btn.innerText = "Load";
            load_btn.onclick = async (evt) => {
                evt.preventDefault();
                loaded_demo = await import(demo.path);
                loaded_demo?.load();
            };
            row.appendChild(name);
            row.appendChild(desc);
            row.appendChild(load_btn);
            container.appendChild(row);
        });
        document.body.insertAdjacentElement("afterbegin", container);
    }
})();
export {};
