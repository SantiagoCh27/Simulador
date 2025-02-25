const { chromium } = require('playwright');

(async () => {
    console.log("🚀 Iniciando navegador...");
    const browser = await chromium.launch({
        headless: false, // Abre el navegador con interfaz gráfica
        args: ['--remote-debugging-port=9222'] // Habilita conexión externa
    });
    const page = await browser.newPage();
    await page.goto('https://www.google.com/search?q=cronometro+online');
    console.log("🔹 Navegador abierto y página cargada.");
})();
