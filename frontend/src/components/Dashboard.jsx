import { useState, useEffect } from 'react';
import MessageCard from './MessageCard';
import { createWebSocket } from '../api/websocket';

const Dashboard = () => {
    const [messages, setMessages] = useState([]);
    const [isOnline, setIsOnline] = useState(false);

    useEffect(() => {
        // Load history
        const apiHost = window.location.hostname === 'localhost' ? 'localhost:8000' : window.location.host;
        const protocol = window.location.protocol;
        fetch(`${protocol}//${apiHost}/messages`)
            .then(res => res.json())
            .then(data => setMessages(data))
            .catch(err => console.error('Error fetching history:', err));

        // WebSocket
        const cleanup = createWebSocket(
            (newMsg) => {
                setMessages(prev => [newMsg, ...prev]);
            },
            (online) => setIsOnline(online)
        );

        return cleanup;
    }, []);

    return (
        <div className="dashboard-container">
            <header className="header">
                <h1>NAVTEX RECEIVER</h1>
                <div className="status-indicator">
                    <span className={`dot ${isOnline ? 'online' : ''}`}></span>
                    {isOnline ? 'LIVE' : 'OFFLINE'}
                </div>
            </header>

            <main>
                {messages.length === 0 ? (
                    <div className="text-secondary text-center py-10">
                        Waiting for messages...
                    </div>
                ) : (
                    messages.map(msg => (
                        <MessageCard key={msg.id} message={msg} />
                    ))
                )}
            </main>
        </div>
    );
};

export default Dashboard;
