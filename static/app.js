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
        latexText.innerHTML = "$$" + data.latex + "$$";
        // Parse CasText and SolutionText line by line to preserve newlines and apply AsciiMath to non-empty lines
        casText.innerHTML = data.parsed_equation.split('\n')
            .map(line => line.trim() ? "`" + line + "`" : "")
            .join('<br>');
        solutionText.innerHTML = data.solution.split('\n')
            .map(line => line.trim() ? "`" + line.trim() + "`" : "")
            .join('<br>');
        outputBox.classList.remove('hidden');

        if (window.MathJax) {
            MathJax.typesetPromise([latexText, casText, solutionText]).catch(function (err) {
                console.error(err.message);
            });
        }

    } catch (error) {
        statusDiv.textContent = `[Engine Fault]: ${error.message}`;
    }
});