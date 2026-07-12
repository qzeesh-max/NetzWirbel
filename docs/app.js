document.addEventListener('DOMContentLoaded', () => {
    const sidebar = document.getElementById('sidebar');
    const toggleBtn = document.getElementById('toggle-sidebar');
    const navTree = document.getElementById('nav-tree');
    const contentBody = document.getElementById('content-body');
    const breadcrumb = document.getElementById('breadcrumb');

    // Toggle Sidebar
    toggleBtn.addEventListener('click', () => {
        sidebar.classList.toggle('collapsed');
    });

    // Configure Marked to use highlight.js
    marked.setOptions({
        highlight: function(code, lang) {
            if (lang && hljs.getLanguage(lang)) {
                return hljs.highlight(code, { language: lang }).value;
            } else {
                return hljs.highlightAuto(code).value;
            }
        },
        breaks: true,
        gfm: true
    });

    let currentNav = [];

    // Fetch Navigation JSON
    fetch('nav.json')
        .then(res => res.json())
        .then(data => {
            currentNav = data;
            renderNav(data);
            
            // Load initial page (first item in the nav tree or read hash)
            const initialHash = window.location.hash.slice(1);
            if (initialHash) {
                loadContent(initialHash);
            } else {
                // Find first non-header item
                let firstItem = null;
                for (const section of data) {
                    if (section.items && section.items.length > 0) {
                        firstItem = section.items[0];
                        break;
                    }
                }
                if (firstItem) loadContent(firstItem.path);
            }
        })
        .catch(err => {
            console.error('Failed to load navigation', err);
            contentBody.innerHTML = '<div class="loading-state">Error loading documentation navigation.</div>';
        });

    function renderNav(data) {
        navTree.innerHTML = '';
        
        data.forEach(section => {
            const group = document.createElement('div');
            group.className = 'nav-group';
            
            const title = document.createElement('div');
            title.className = 'nav-title';
            title.textContent = section.title;
            group.appendChild(title);
            
            section.items.forEach(item => {
                const link = document.createElement('a');
                link.className = 'nav-item';
                link.textContent = item.name;
                link.href = `#${item.path}`;
                link.dataset.path = item.path;
                
                link.addEventListener('click', (e) => {
                    e.preventDefault();
                    window.location.hash = item.path;
                    loadContent(item.path);
                });
                
                group.appendChild(link);
            });
            
            navTree.appendChild(group);
        });
    }

    function updateActiveNav(path) {
        document.querySelectorAll('.nav-item').forEach(el => {
            if (el.dataset.path === path) {
                el.classList.add('active');
                
                // Update breadcrumb
                let sectionTitle = '';
                for (const section of currentNav) {
                    const match = section.items.find(i => i.path === path);
                    if (match) {
                        sectionTitle = section.title;
                        break;
                    }
                }
                breadcrumb.textContent = `${sectionTitle} / ${el.textContent}`;
            } else {
                el.classList.remove('active');
            }
        });
    }

    function loadContent(path) {
        contentBody.innerHTML = '<div class="loading-state">Loading content...</div>';
        updateActiveNav(path);
        
        // Remove .md if present and construct URL to content folder
        const normalizedPath = path.endsWith('.md') ? path : `${path}.md`;
        
        fetch(`content/${normalizedPath}`)
            .then(res => {
                if (!res.ok) throw new Error('Not found');
                return res.text();
            })
            .then(markdown => {
                contentBody.innerHTML = marked.parse(markdown);
                
                // Add logo to H1
                const h1 = contentBody.querySelector('h1');
                if (h1) {
                    const wrapper = document.createElement('div');
                    wrapper.style.display = 'flex';
                    wrapper.style.alignItems = 'center';
                    wrapper.style.gap = '1rem';
                    wrapper.style.marginBottom = '2rem';
                    wrapper.style.borderBottom = '1px solid #333';
                    wrapper.style.paddingBottom = '1rem';
                    
                    const img = document.createElement('img');
                    img.src = 'logo.png';
                    img.style.height = '48px';
                    img.alt = 'NetzWirbel Logo';
                    
                    h1.parentNode.insertBefore(wrapper, h1);
                    wrapper.appendChild(img);
                    wrapper.appendChild(h1);
                    h1.style.borderBottom = 'none';
                    h1.style.paddingBottom = '0';
                    h1.style.marginBottom = '0';
                }

                // Fix GitHub alerts [!NOTE], [!WARNING], etc.
                contentBody.querySelectorAll('blockquote').forEach(bq => {
                    const firstP = bq.querySelector('p');
                    if (firstP) {
                        const html = firstP.innerHTML;
                        const match = html.match(/^\[!(NOTE|TIP|IMPORTANT|WARNING|CAUTION)\](?:<br>)?/i);
                        if (match) {
                            const type = match[1].toLowerCase();
                            bq.classList.add('gh-alert', `gh-alert-${type}`);
                            firstP.innerHTML = html.substring(match[0].length);
                            
                            const titleDiv = document.createElement('div');
                            titleDiv.className = 'gh-alert-title';
                            titleDiv.textContent = match[1].toUpperCase();
                            bq.insertBefore(titleDiv, firstP);
                        }
                    }
                });

                // Highlight block execution
                document.querySelectorAll('pre code').forEach((block) => {
                    hljs.highlightElement(block);
                });
                
                // Reset scroll
                document.querySelector('.content-wrapper').scrollTop = 0;
            })
            .catch(err => {
                contentBody.innerHTML = `
                    <div style="text-align:center; padding: 40px;">
                        <h2>404 - Page Not Found</h2>
                        <p style="color:var(--text-secondary); margin-top:10px;">The requested document at <code>content/${normalizedPath}</code> could not be found.</p>
                    </div>
                `;
            });
    }
    
    // Listen for back/forward browser navigation
    window.addEventListener('hashchange', () => {
        const hash = window.location.hash.slice(1);
        if (hash) {
            loadContent(hash);
        }
    });
});
