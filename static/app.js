document.getElementById('upload-form').addEventListener('submit', async (e) => {
    e.preventDefault();

    const imageInput = document.getElementById('equation-image');
    const statusDiv = document.getElementById('status');
    const outputBox = document.getElementById('output-box');
    const solutionText = document.getElementById('solution-text');

    const formData = new FormData();
    formData.append('file', imageInput.files[0]);

    statusDiv.textContent = "[System] Uploading and processing tensor...";
    outputBox.classList.add('hidden');

    try {
        const response = await fetch('/solve', {
            method: 'POST',
            body: formData
        });

        if (!response.ok) throw new Error(`HTTP error! status: ${response.status}`);

        const data = await response.json();

        const solutionText = document.getElementById('solution-text');
        const latexText = document.getElementById('latex-text');
        const casText = document.getElementById('cas-text');

        statusDiv.textContent = "";
        latexText.textContent = data.latex;
        casText.textContent = data.parsed_equation;
        solutionText.textContent = data.solution;
        outputBox.classList.remove('hidden');

    } catch (error) {
        statusDiv.textContent = `[Engine Fault]: ${error.message}`;
    }
});