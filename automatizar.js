const { chromium } = require('playwright');

(async () => {
    console.time("⏳ Tiempo de ejecución"); // Iniciar medición del tiempo

    // Conectarse al navegador ya abierto en el puerto 9222
    const browser = await chromium.connectOverCDP('http://localhost:9222');
    
    // Obtener el contexto del navegador
    const context = browser.contexts()[0];
    
    // Obtener la primera pestaña (página)
    const page = context.pages()[0];

    console.log("🔹 Conectado al navegador abierto.");

    // Ir a la página del cronómetro si aún no está en ella
    if (!page.url().includes("cronometro+online")) {
        await page.goto('https://www.google.com/search?q=cronometro+online');
        await page.waitForSelector('text=Cronómetro', { timeout: 1000 }); // Espera el elemento en lugar de usar tiempo fijo
    }

    // Hacer los clics y la prueba
    await page.mouse.click(400, 535); // Play cronómetro
    await page.waitForTimeout(100); // Reducido de 500 a 100ms
    await page.mouse.click(368, 50); // Click en caja de texto
    await page.keyboard.press('Control+A'); // Seleccionar todo
    await page.keyboard.press('Delete'); // Eliminar contenido
    await page.keyboard.type('1'); // Escribir "1"
    await page.waitForTimeout(100); // Reducido de 500 a 100ms
    await page.mouse.click(50, 350); // Click en botón (ajustar coordenadas según sea necesario)
    await page.waitForTimeout(100); // Reducido de 500 a 100ms
    await page.mouse.click(220, 553); // Click en otro botón (ajustar coordenadas según sea necesario)
    await page.waitForTimeout(500); // Reducido de 2000 a 500ms

    console.log("🔹 Prueba completada.");
    
    console.timeEnd("⏳ Tiempo de ejecución"); // Finaliza la medición de tiempo
})();
