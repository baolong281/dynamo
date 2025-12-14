// Node management
let nodes = [];

// Context storage for keys (to use in subsequent PUTs)
let keyContexts = new Map(); // Map<key, context[]>

// Load nodes from localStorage on page load
window.addEventListener('DOMContentLoaded', () => {
    loadNodes();
    renderNodes();
    updateNodeSelects();
});

function loadNodes() {
    const saved = localStorage.getItem('dynamoNodes');
    if (saved) {
        nodes = JSON.parse(saved);
    }
}

function saveNodes() {
    localStorage.setItem('dynamoNodes', JSON.stringify(nodes));
}

function addNode() {
    const host = document.getElementById('nodeHost').value.trim();
    const port = document.getElementById('nodePort').value.trim();

    if (!host || !port) {
        showNotification('Please enter both host and port', 'error');
        return;
    }

    const nodeAddress = `${host}:${port}`;
    
    if (nodes.includes(nodeAddress)) {
        showNotification('Node already exists', 'warning');
        return;
    }

    nodes.push(nodeAddress);
    saveNodes();
    renderNodes();
    updateNodeSelects();
    showNotification(`Node ${nodeAddress} added successfully`, 'success');
}

function removeNode(nodeAddress) {
    nodes = nodes.filter(n => n !== nodeAddress);
    saveNodes();
    renderNodes();
    updateNodeSelects();
    showNotification(`Node ${nodeAddress} removed`, 'info');
}

function renderNodes() {
    const nodesList = document.getElementById('nodesList');
    
    if (nodes.length === 0) {
        nodesList.innerHTML = '<div class="empty-state">No nodes configured. Add a node to get started.</div>';
        return;
    }

    nodesList.innerHTML = nodes.map(node => `
        <div class="node-item">
            <div class="node-info">
                <div class="node-status"></div>
                <span class="node-address">${node}</span>
            </div>
            <button class="btn-remove" onclick="removeNode('${node}')" title="Remove node">×</button>
        </div>
    `).join('');
}

function updateNodeSelects() {
    const putSelect = document.getElementById('putNodeSelect');
    const getSelect = document.getElementById('getNodeSelect');

    const options = nodes.map(node => `<option value="${node}">${node}</option>`).join('');
    const defaultOption = '<option value="">Select a node...</option>';

    putSelect.innerHTML = defaultOption + options;
    getSelect.innerHTML = defaultOption + options;

    const membershipSelect = document.getElementById('membershipNodeSelect');
    if (membershipSelect) {
        membershipSelect.innerHTML = defaultOption + options;
    }
}

async function fetchMembership() {
    const node = document.getElementById('membershipNodeSelect').value;
    const listContainer = document.getElementById('membershipList');

    if (!node) {
        showNotification('Please select a node to query', 'error');
        return;
    }

    try {
        const url = `http://${node}/admin/membership`;
        const response = await fetch(url, {
            method: 'POST',
            mode: 'cors'
        });

        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }

        const data = await response.json();
        renderMembership(data);
        
        // Only show success notification if manually triggered (not polling)
        if (!pollingInterval) {
            showNotification('Membership status updated', 'success');
        }

    } catch (error) {
        console.error('Membership fetch error:', error);
        // Only show error in container if list is empty
        if (!listContainer.hasChildNodes() || listContainer.innerHTML.includes('empty-state')) {
             listContainer.innerHTML = `<div class="error-state">Failed to fetch membership: ${error.message}</div>`;
        }
        
        // Don't spam notifications if polling
        if (!pollingInterval) {
            showNotification(`Failed to fetch membership: ${error.message}`, 'error');
        }
    }
}

function renderMembership(data) {
    const listContainer = document.getElementById('membershipList');
    
    // Check if data is a map/object (ClusterState)
    // The data structure based on gossip.h is map<string, NodeState>
    // checking if we received an object/map
    
    if (!data || typeof data !== 'object') {
        listContainer.innerHTML = '<div class="empty-state">Invalid data format received</div>';
        return;
    }

    const nodes = Object.values(data);
    
    if (nodes.length === 0) {
        listContainer.innerHTML = '<div class="empty-state">No members in cluster</div>';
        return;
    }

    // Sort nodes: self/local first, then by ID
    nodes.sort((a, b) => a.id_ > b.id_ ? 1 : -1);

    listContainer.innerHTML = nodes.map(node => `
        <div class="membership-card ${node.status_ === 0 ? 'status-active' : 'status-killed'}">
            <div class="member-header">
                <span class="member-id" title="${node.id_}">${node.id_.substring(0, 8)}...</span>
                <span class="member-status badge">${node.status_ === 0 ? 'ACTIVE' : 'KILLED'}</span>
            </div>
            <div class="member-details">
                <div class="detail-row">
                    <span class="label">Address:</span>
                    <span class="value">${node.address_}:${node.port_}</span>
                </div>
                <div class="detail-row">
                    <span class="label">Incarnation:</span>
                    <span class="value">${node.incarnation_}</span>
                </div>
            </div>
        </div>
    `).join('');
}

// Live Update Logic
let pollingInterval = null;

function toggleLiveUpdate() {
    const toggle = document.getElementById('liveUpdateToggle');
    const isEnabled = toggle.checked;
    
    if (isEnabled) {
        fetchMembership(); // Fetch immediately
        pollingInterval = setInterval(fetchMembership, 2000); // 2 second interval
        showNotification('Live updates enabled', 'success');
    } else {
        if (pollingInterval) {
            clearInterval(pollingInterval);
            pollingInterval = null;
        }
        showNotification('Live updates disabled', 'info');
    }
}

// Base64 encoding/decoding utilities
function stringToBase64(str) {
    // Convert string to UTF-8 bytes, then to base64
    return btoa(unescape(encodeURIComponent(str)));
}

function base64ToString(base64) {
    // Convert base64 to UTF-8 bytes, then to string
    try {
        return decodeURIComponent(escape(atob(base64)));
    } catch (e) {
        // If decoding fails, return raw bytes as string
        return atob(base64);
    }
}

// API Operations
async function executePut() {
    const node = document.getElementById('putNodeSelect').value;
    const key = document.getElementById('putKey').value.trim();
    const value = document.getElementById('putValue').value;

    if (!node) {
        showNotification('Please select a target node', 'error');
        return;
    }

    if (!key) {
        showNotification('Please enter a key', 'error');
        return;
    }

    try {
        // Encode data to base64
        const dataBase64 = stringToBase64(value);
        
        // Get context from previous GET operation (if any), otherwise use empty string
        const contexts = keyContexts.get(key);
        const contextBase64 = (contexts && contexts.length > 0) ? contexts[0] : "";

        // Build request body
        const requestBody = {
            key: key,
            data: dataBase64,
            context: contextBase64
        };

        const url = `http://${node}/put`;
        const headers = {
            'Content-Type': 'application/json',
            'Key': key
        };

        const response = await fetch(url, {
            method: 'POST',
            headers: headers,
            body: JSON.stringify(requestBody),
            mode: 'cors'
        });

        // Display request and response
        await displayRequestResponse({
            method: 'POST',
            url: url,
            headers: headers,
            body: JSON.stringify(requestBody, null, 2)
        }, response, 'PUT', key);
        
        if (response.ok) {
            showNotification(`Successfully stored key: ${key}`, 'success');
            // Clear the context after successful PUT
            keyContexts.delete(key);
        }
    } catch (error) {
        displayError(error, 'PUT');
    }
}

async function executeGet() {
    const node = document.getElementById('getNodeSelect').value;
    const key = document.getElementById('getKey').value.trim();

    if (!node) {
        showNotification('Please select a target node', 'error');
        return;
    }

    if (!key) {
        showNotification('Please enter a key', 'error');
        return;
    }

    try {
        // Build request body
        const requestBody = {
            key: key
        };

        const url = `http://${node}/get`;
        const headers = {
            'Content-Type': 'application/json',
            'Key': key
        };

        const response = await fetch(url, {
            method: 'POST',
            headers: headers,
            body: JSON.stringify(requestBody),
            mode: 'cors'
        });

        // Display request and response
        await displayRequestResponse({
            method: 'POST',
            url: url,
            headers: headers,
            body: JSON.stringify(requestBody, null, 2)
        }, response, 'GET', key);
    } catch (error) {
        displayError(error, 'GET');
    }
}

async function displayRequestResponse(request, response, operation, key = null) {
    const responseSection = document.getElementById('responseSection');
    
    // Request elements
    const requestStatus = document.getElementById('requestStatus');
    const requestHeaders = document.getElementById('requestHeaders');
    const requestBody = document.getElementById('requestBody');
    
    // Response elements
    const responseStatus = document.getElementById('responseStatus');
    const responseHeaders = document.getElementById('responseHeaders');
    const responseBody = document.getElementById('responseBody');

    // Show response section
    responseSection.style.display = 'block';
    responseSection.scrollIntoView({ behavior: 'smooth', block: 'nearest' });

    // === Display Request ===
    const requestStatusText = `${request.method} ${request.url}`;
    requestStatus.innerHTML = `<pre style="display: inline; margin: 0; font-family: monospace; white-space: pre-wrap;">${requestStatusText}</pre>`;
    requestStatus.className = 'response-status';
    requestStatus.style.background = '#f0f0f0';
    requestStatus.style.color = '#333';

    // Display request headers
    requestHeaders.textContent = JSON.stringify(request.headers, null, 2);

    // Display request body
    requestBody.textContent = request.body;

    // === Display Response ===
    const statusText = `HTTP Response\nStatus: ${response.status} ${response.statusText}\nURL: ${response.url}\nOperation: ${operation}`;
    const statusIcon = response.ok ? '✓' : '✗';
    responseStatus.innerHTML = `<span>${statusIcon}</span> <pre style="display: inline; margin: 0; font-family: monospace; white-space: pre-wrap;">${statusText}</pre>`;
    responseStatus.className = `response-status status-${response.status}`;

    // Display response headers
    const headers = {};
    response.headers.forEach((value, key) => {
        headers[key] = value;
    });
    const headersText = Object.keys(headers).length > 0 
        ? JSON.stringify(headers, null, 2)
        : '(no headers)';
    responseHeaders.textContent = headersText;

    // Display response body
    const bodyText = await response.text();
    
    if (!bodyText) {
        responseBody.textContent = '(empty body)';
        return;
    }

    let displayText = `Length: ${bodyText.length} bytes\n\n`;

    // Try to parse as JSON for GET responses
    if (operation === 'GET' && response.ok) {
        try {
            const jsonData = JSON.parse(bodyText);

            console.log(jsonData);
            
            // Store contexts for this key
            if (key && jsonData.values && Array.isArray(jsonData.values)) {
                const contexts = jsonData.values.map(item => item.context);
                console.log(contexts);
                keyContexts.set(key, contexts);
                displayText += `Stored ${contexts.length} context(s) for key: ${key}\n\n`;
            }

            displayText += 'Raw JSON:\n';
            displayText += JSON.stringify(jsonData, null, 2);
            displayText += '\n\n';

            // Decode and display the data
            if (jsonData.values && Array.isArray(jsonData.values)) {
                displayText += `Decoded Values (${jsonData.values.length} versions):\n`;
                displayText += '─'.repeat(50) + '\n';
                
                jsonData.values.forEach((item, index) => {
                    displayText += `\nVersion ${index + 1}:\n`;
                    
                    // Decode data
                    if (item.data) {
                        try {
                            const decodedData = base64ToString(item.data);
                            displayText += `  Data: ${decodedData}\n`;
                        } catch (e) {
                            displayText += `  Data (base64): ${item.data}\n`;
                            displayText += `  (Decoding error: ${e.message})\n`;
                        }
                    }
                    
                    // Show context (kept as base64 for subsequent PUTs)
                    if (item.context) {
                        displayText += `  Context (base64): ${item.context}\n`;
                    }
                });
            }
        } catch (e) {
            displayText += bodyText;
        }
    } else {
        displayText += bodyText;
    }

    responseBody.textContent = displayText;
}

function displayError(error, operation) {
    const responseSection = document.getElementById('responseSection');
    const responseStatus = document.getElementById('responseStatus');
    const responseHeaders = document.getElementById('responseHeaders');
    const responseBody = document.getElementById('responseBody');

    responseSection.style.display = 'block';
    responseSection.scrollIntoView({ behavior: 'smooth', block: 'nearest' });

    const errorDetails = `Network Error\nOperation: ${operation}\nError Type: ${error.name}\nError Message: ${error.message}`;
    responseStatus.innerHTML = `<span>✗</span> <pre style="display: inline; margin: 0; font-family: monospace; white-space: pre-wrap;">${errorDetails}</pre>`;
    responseStatus.className = 'response-status status-400';

    responseHeaders.textContent = 'N/A - Request did not complete';
    
    const errorBody = `Error Details:
Name: ${error.name}
Message: ${error.message}

Possible causes:
- Server not running on the specified node
- CORS policy blocking the request (try running UI from a local server)
- Network connectivity issues
- Invalid host/port configuration`;
    
    responseBody.textContent = errorBody;

    showNotification(`Request failed: ${error.message}`, 'error');
}

function clearResponse() {
    document.getElementById('responseSection').style.display = 'none';
}

// Notification system
function showNotification(message, type = 'info') {
    // Create notification element
    const notification = document.createElement('div');
    notification.className = `notification notification-${type}`;
    notification.textContent = message;
    
    // Style the notification
    Object.assign(notification.style, {
        position: 'fixed',
        top: '20px',
        right: '20px',
        padding: '1rem 1.5rem',
        borderRadius: '10px',
        fontWeight: '600',
        fontSize: '0.9rem',
        zIndex: '1000',
        animation: 'slideInRight 0.3s ease-out',
        boxShadow: '0 4px 20px rgba(0, 0, 0, 0.4)',
        maxWidth: '400px'
    });

    // Type-specific styling
    const colors = {
        success: { bg: 'rgba(16, 185, 129, 0.2)', border: 'rgba(16, 185, 129, 0.6)', color: '#10b981' },
        error: { bg: 'rgba(239, 68, 68, 0.2)', border: 'rgba(239, 68, 68, 0.6)', color: '#ef4444' },
        warning: { bg: 'rgba(245, 158, 11, 0.2)', border: 'rgba(245, 158, 11, 0.6)', color: '#f59e0b' },
        info: { bg: 'rgba(59, 130, 246, 0.2)', border: 'rgba(59, 130, 246, 0.6)', color: '#3b82f6' }
    };

    const style = colors[type] || colors.info;
    notification.style.background = style.bg;
    notification.style.border = `1px solid ${style.border}`;
    notification.style.color = style.color;

    document.body.appendChild(notification);

    // Auto-remove after 3 seconds
    setTimeout(() => {
        notification.style.animation = 'slideOutRight 0.3s ease-out';
        setTimeout(() => notification.remove(), 300);
    }, 3000);
}

// Add notification animations to CSS dynamically
const style = document.createElement('style');
style.textContent = `
    @keyframes slideInRight {
        from {
            opacity: 0;
            transform: translateX(100px);
        }
        to {
            opacity: 1;
            transform: translateX(0);
        }
    }
    @keyframes slideOutRight {
        from {
            opacity: 1;
            transform: translateX(0);
        }
        to {
            opacity: 0;
            transform: translateX(100px);
        }
    }
`;
document.head.appendChild(style);
