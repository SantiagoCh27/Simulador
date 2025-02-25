// coordinates-finder.js
const { chromium } = require('playwright');

(async () => {
    // Conectarse al navegador existente
    const browser = await chromium.connectOverCDP('http://localhost:9222');
    const context = browser.contexts()[0];
    const page = context.pages()[0];

    console.log("🔹 Conectado al navegador. Presiona Ctrl+C para salir.");

    // Escuchar eventos del mouse
    await page.evaluate(() => {
        document.addEventListener('mousemove', (e) => {
            // Crear o actualizar el elemento de visualización
            let display = document.getElementById('coordinate-display');
            if (!display) {
                display = document.createElement('div');
                display.id = 'coordinate-display';
                display.style.position = 'fixed';
                display.style.top = '10px';
                display.style.left = '10px';
                display.style.backgroundColor = 'rgba(0, 0, 0, 0.8)';
                display.style.color = 'white';
                display.style.padding = '10px';
                display.style.borderRadius = '5px';
                display.style.zIndex = '10000';
                document.body.appendChild(display);
            }
            display.textContent = `X: ${e.clientX}, Y: ${e.clientY}`;
        });

        document.addEventListener('click', (e) => {
            console.log(`Coordenadas del clic: X: ${e.clientX}, Y: ${e.clientY}`);
        });
    });

    // Mantener el script ejecutándose
    await new Promise(() => {});
})();