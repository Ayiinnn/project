const express = require('express');
const mammoth = require('mammoth');
const fs = require('fs');
const path = require('path');
const app = express();

//根路径
app.get('/', (req, res) => {
    res.sendFile(path.join(__dirname, 'app.html'));
});
//获取文件
app.get('/document/:index', async (req, res) => {
    const index = req.params.index;
    const filePath = path.join(__dirname, `${index}.docx`);

    try {
        if (!fs.existsSync(filePath)) {
            return res.status(404).send('Document not found');
        }

        const data = fs.readFileSync(filePath);
        const result = await mammoth.convertToHtml({buffer: data});
        res.send(result.value); // Send HTML content to client
    } catch (error) {
        res.status(500).send('Error reading document');
    }
});

app.listen(3000, () => {
    console.log('Server is running on port 3000');
});
