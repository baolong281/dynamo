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

let ringPollingInterval = null;

window.addEventListener('resize', resizeCanvas);
window.addEventListener('DOMContentLoaded', () => {
    resizeCanvas();
    populateNodeSelector();
    fetchRingData();
    setupInteractions();
    startRingPolling();
});

function populateNodeSelector() {
    const globalSelect = document.getElementById('globalNodeSelect');
    if (!globalSelect) return;
    
    const savedNodes = localStorage.getItem('dynamoNodes');
    if (!savedNodes) return;
    
    const nodes = JSON.parse(savedNodes);
    const options = nodes.map(node => `<option value="${node}">${node}</option>`).join('');
    const defaultOption = '<option value="">Select a node...</option>';
    
    globalSelect.innerHTML = defaultOption + options;
    
    // Restore from localStorage
    const savedNode = localStorage.getItem('selectedDynamoNode');
    if (savedNode && nodes.includes(savedNode)) {
        globalSelect.value = savedNode;
    } else if (nodes.length > 0) {
        globalSelect.value = nodes[0];
        localStorage.setItem('selectedDynamoNode', nodes[0]);
    }
}

function getSelectedNode() {
    const globalSelect = document.getElementById('globalNodeSelect');
    if (globalSelect && globalSelect.value) {
        return globalSelect.value;
    }
    // Fallback to localStorage
    return localStorage.getItem('selectedDynamoNode') || '';
}

function onGlobalNodeChange() {
    const node = getSelectedNode();
    if (node) {
        localStorage.setItem('selectedDynamoNode', node);
        fetchRingData();
    }
}

function resizeCanvas() {
    const parent = canvas.parentElement;
    canvas.width = parent.clientWidth;
    canvas.height = parent.clientHeight || 600;
    drawRing();
}

async function fetchRingData() {
    try {
        const targetHost = getSelectedNode();
        
        if (!targetHost) {
            return; // No node selected
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
    }
}

function startRingPolling() {
    if (ringPollingInterval) return;
    ringPollingInterval = setInterval(fetchRingData, 3000); // 3 second interval
}

function processData(data) {
    // Expected format: array of objects with "parent_ -> getId()" key
    // Map this weird key to "parentId"
    
    // Sort logic handled in backend? Usually ring is sorted by position.
    // Let's ensure raw data is sorted by position just in case for drawing lines/arcs.
    
    const statusMap = new Map(); // parentId -> boolean (isActive)
    
    ringData = data.map(item => {
        // Handle both possible key formats just in case they fix it, or the raw weird one
        const parentId = item["parent_ -> getId()"] || item.parentId || item.parent_id || "unknown";
        const position = BigInt(item.position_);
        
        // Capture status
        // The key is likely "parent_ -> isActive()" as per user request
        if (item.hasOwnProperty("parent_ -> isActive()")) {
            statusMap.set(parentId, item["parent_ -> isActive()"]);
        } else if (item.hasOwnProperty("isActive")) {
             statusMap.set(parentId, item["isActive"]);
        } else {
            // Default to true if not found, or maybe false? Let's assume true for backward compat? 
            // Or better, check if we have seen it before.
            if (!statusMap.has(parentId)) statusMap.set(parentId, true);
        }

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
    renderNodeStatusList(uniqueParents, statusMap);
}

function renderNodeStatusList(parents, statusMap) {
    const container = document.getElementById('nodeStatusList');
    if (!container) return;

    if (parents.length === 0) {
        container.innerHTML = '<div class="empty-state">No nodes found</div>';
        return;
    }

    container.innerHTML = parents.map(pid => {
        const isActive = statusMap.get(pid);
        const color = parentNodes.get(pid);
        
        return `
        <div class="membership-card ${isActive ? 'status-active' : 'status-killed'}">
            <div class="member-header">
                <span class="member-id" style="color:${color}">${pid}</span>
                <span class="member-status badge">${isActive ? 'ACTIVE' : 'OUTAGE'}</span>
            </div>
            <div class="member-details">
                <div class="detail-row">
                    <span class="label">Status:</span>
                    <span class="value">${isActive ? 'Operating Normally' : 'Suspected Down'}</span>
                </div>
            </div>
        </div>
        `;
    }).join('');
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
