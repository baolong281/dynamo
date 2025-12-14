const canvas = document.getElementById('ringCanvas');
const ctx = canvas.getContext('2d');
const tooltip = document.getElementById('tooltip');

// State
let ringData = [];
let parentNodes = new Map(); // id -> color
let scale = 1;
let offset = { x: 0, y: 0 };
let isDragging = false;
let startPan = { x: 0, y: 0 };
const MAX_UINT64 = 18446744073709551615n;

// Configuration
const BASE_RADIUS = 300;
const NODE_RADIUS = 4;
const HOVER_RADIUS = 6;

// Vibrant Colors for Parents (contrast against white/black)
const COLORS = [
    '#e11d48', // Ring Red
    '#2563eb', // Ring Blue
    '#16a34a', // Ring Green
    '#d97706', // Ring Amber
    '#9333ea', // Ring Purple
    '#0891b2', // Ring Cyan
    '#db2777', // Ring Pink
    '#4f46e5', // Ring Indigo
];

window.addEventListener('resize', resizeCanvas);
window.addEventListener('DOMContentLoaded', () => {
    resizeCanvas();
    fetchRingData();
    setupInteractions();
});

function resizeCanvas() {
    const parent = canvas.parentElement;
    canvas.width = parent.clientWidth;
    canvas.height = parent.clientHeight || 600;
    drawRing();
}

async function fetchRingData() {
    try {
        // In real dev, we might need to know which node to ask. 
        // For now, assume localhost:8080 or try to get from localStorage if available? 
        // Or just try specific common ports.
        // Simple fallback logic: try localhost:8080, if fail try 8081.
        // Actually, let's just pick one from localStorage if available, else default.
        
        let targetHost = 'localhost:8080';
        const savedNodes = localStorage.getItem('dynamoNodes');
        if (savedNodes) {
            const nodes = JSON.parse(savedNodes);
            if (nodes.length > 0) targetHost = nodes[0];
        }

        const response = await fetch(`http://${targetHost}/admin/ring`, {
            method: 'POST',
            mode: 'cors'
        });

        if (!response.ok) throw new Error('Failed to fetch ring data');

        const data = await response.json();
        processData(data);
        drawRing();

    } catch (error) {
        console.error('Error fetching ring:', error);
        alert('Failed to fetch ring data. using mock data for viz testing if empty.');
        // Optional: Generate mock data for visualization testing if fetch fails?
        // keeping it clean for now.
    }
}

function processData(data) {
    // Expected format: array of objects with "parent_ -> getId()" key
    // Map this weird key to "parentId"
    
    // Sort logic handled in backend? Usually ring is sorted by position.
    // Let's ensure raw data is sorted by position just in case for drawing lines/arcs.
    
    ringData = data.map(item => {
        // Handle both possible key formats just in case they fix it, or the raw weird one
        const parentId = item["parent_ -> getId()"] || item.parentId || item.parent_id || "unknown";
        const position = BigInt(item.position_);
        
        return {
            id: item.id_,
            parentId: parentId,
            position: position,
            // Calculate normalized angle [0, 2PI]
            angle: Number((position * 1000000n) / MAX_UINT64) / 1000000 * (2 * Math.PI)
        };
    }).sort((a, b) => (a.position < b.position) ? -1 : 1);

    // Assign colors to unique parents
    parentNodes.clear();
    const uniqueParents = [...new Set(ringData.map(d => d.parentId))].sort();
    uniqueParents.forEach((pid, index) => {
        parentNodes.set(pid, COLORS[index % COLORS.length]);
    });

    // Update Stats
    document.getElementById('vnodeCount').textContent = ringData.length;
    document.getElementById('pnodeCount').textContent = uniqueParents.length;

    updateLegend();
}

function updateLegend() {
    const legend = document.getElementById('ringLegend');
    legend.innerHTML = '';
    
    parentNodes.forEach((color, pid) => {
        const item = document.createElement('div');
        item.className = 'legend-item';
        item.innerHTML = `
            <span class="legend-color" style="background: ${color}"></span>
            <span class="legend-label">${pid}</span>
        `;
        legend.appendChild(item);
    });
}

function drawRing() {
    if (!ctx) return;

    // clear
    ctx.clearRect(0, 0, canvas.width, canvas.height);

    const cx = canvas.width / 2 + offset.x;
    const cy = canvas.height / 2 + offset.y;
    const r = BASE_RADIUS * scale;

    // Draw Main Ring Circle
    ctx.beginPath();
    ctx.arc(cx, cy, r, 0, 2 * Math.PI);
    ctx.strokeStyle = '#000';
    ctx.lineWidth = 2;
    ctx.stroke();

    // Draw Nodes
    ringData.forEach(node => {
        const x = cx + r * Math.cos(node.angle - Math.PI / 2); // -PI/2 to start at 12 o'clock
        const y = cy + r * Math.sin(node.angle - Math.PI / 2);

        ctx.beginPath();
        ctx.arc(x, y, NODE_RADIUS * (scale < 1 ? 1 : Math.sqrt(scale)), 0, 2 * Math.PI);
        ctx.fillStyle = parentNodes.get(node.parentId) || '#000';
        ctx.fill();
        
        // Minimalist border for dots
        // ctx.stroke(); 
    });
}

// Interaction Logic
function setupInteractions() {
    // Zoom
    canvas.addEventListener('wheel', (e) => {
        e.preventDefault();
        const delta = e.deltaY > 0 ? 0.9 : 1.1;
        applyZoom(delta);
    });

    // Pan
    canvas.addEventListener('mousedown', (e) => {
        isDragging = true;
        startPan = { x: e.clientX - offset.x, y: e.clientY - offset.y };
        canvas.style.cursor = 'grabbing';
    });

    window.addEventListener('mousemove', (e) => {
        if (isDragging) {
            offset.x = e.clientX - startPan.x;
            offset.y = e.clientY - startPan.y;
            drawRing();
        }
        handleHover(e);
    });

    window.addEventListener('mouseup', () => {
        isDragging = false;
        canvas.style.cursor = 'default';
    });
}

function applyZoom(factor) {
    scale *= factor;
    scale = Math.max(0.1, Math.min(scale, 20)); // Limits
    drawRing();
}

function zoomIn() { applyZoom(1.2); }
function zoomOut() { applyZoom(0.8); }
function resetZoom() {
    scale = 1;
    offset = { x: 0, y: 0 };
    drawRing();
}

function handleHover(e) {
    const rect = canvas.getBoundingClientRect();
    const mouseX = e.clientX - rect.left;
    const mouseY = e.clientY - rect.top;

    const cx = canvas.width / 2 + offset.x;
    const cy = canvas.height / 2 + offset.y;
    const r = BASE_RADIUS * scale;

    // Check collision with any node
    // Optimization: Only check if mouse is roughly on the ring radius
    const distFromCenter = Math.sqrt((mouseX - cx) ** 2 + (mouseY - cy) ** 2);
    if (Math.abs(distFromCenter - r) > 20 * scale) {
        tooltip.style.display = 'none';
        return;
    }

    // Find closest node
    let closestNode = null;
    let minDist = Infinity;
    const threshold = (NODE_RADIUS + 4) * Math.max(1, scale);

    for (const node of ringData) {
        const nx = cx + r * Math.cos(node.angle - Math.PI / 2);
        const ny = cy + r * Math.sin(node.angle - Math.PI / 2);
        
        const dist = Math.sqrt((mouseX - nx) ** 2 + (mouseY - ny) ** 2);
        if (dist < threshold && dist < minDist) {
            minDist = dist;
            closestNode = node;
        }
    }

    if (closestNode) {
        tooltip.style.display = 'block';
        tooltip.style.left = (e.clientX + 10) + 'px';
        tooltip.style.top = (e.clientY + 10) + 'px';
        tooltip.innerHTML = `
            <strong>Node:</strong> <span style="font-family:monospace">${closestNode.parentId}</span><br>
            <strong>Tokens:</strong> ${closestNode.position.toString().substring(0, 10)}...<br>
            <strong>VNode ID:</strong> <span style="font-size:0.8em">${closestNode.id}</span>
        `;
    } else {
        tooltip.style.display = 'none';
    }
}
