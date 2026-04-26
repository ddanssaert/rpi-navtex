export const createWebSocket = (onMessage, onStatusChange) => {
    const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
    const wsUrl = `${protocol}//${window.location.host}/ws`;

    let ws;
    let reconnectTimer;

    const connect = () => {
        ws = new WebSocket(wsUrl);

        ws.onopen = () => {
            console.log('WebSocket connected');
            onStatusChange(true);
            clearTimeout(reconnectTimer);
        };

        ws.onmessage = (event) => {
            try {
                const data = JSON.parse(event.data);
                onMessage(data);
            } catch (err) {
                console.error('Error parsing WS message:', err);
            }
        };

        ws.onclose = () => {
            console.log('WebSocket disconnected');
            onStatusChange(false);
            reconnectTimer = setTimeout(connect, 3000); // Reconnect every 3s
        };

        ws.onerror = (err) => {
            console.error('WebSocket error:', err);
            ws.close();
        };
    };

    connect();

    return () => {
        clearTimeout(reconnectTimer);
        if (ws) ws.close();
    };
};
