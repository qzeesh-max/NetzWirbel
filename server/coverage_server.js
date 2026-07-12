/*
 * Copyright (C) 2026 NetzWirbel Contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

const express = require('express');
const path = require('path');
const fs = require('fs');

const app = express();
const PORT = process.env.PORT || 8080;

const coverageDir = path.join(__dirname, '../build/tests/coverage_html');

app.use((req, res, next) => {
    if (req.path === '/') {
        if (!fs.existsSync(path.join(coverageDir, 'index.html'))) {
            return res.status(404).send('Coverage report not found. Please run "npm run test:coverage" first.');
        }
    }
    next();
});

app.use(express.static(coverageDir));

app.listen(PORT, () => {
    console.log(`Coverage server is running on http://localhost:${PORT}`);
    console.log(`Serving from ${coverageDir}`);
});
