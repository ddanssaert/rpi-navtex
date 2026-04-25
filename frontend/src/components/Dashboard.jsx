import { useState, useEffect } from 'react';
import MessageCard from './MessageCard';
import FilterBar from './FilterBar';
import useFilters from '../hooks/useFilters';
import { createWebSocket } from '../api/websocket';

const Dashboard = () => {
    const [messages, setMessages] = useState([]);
    const [isOnline, setIsOnline] = useState(false);
    const { filters, toggleStation, toggleType, isFiltered } = useFilters();

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

    const filteredMessages = messages.filter(isFiltered);

    return (
        <div className="dashboard-container">
            <header className="header">
                <h1>NAVTEX RECEIVER</h1>
                <div className="status-indicator">
                    <span className={`dot ${isOnline ? 'online' : ''}`}></span>
                    {isOnline ? 'LIVE' : 'OFFLINE'}
                </div>
            </header>

            <FilterBar
                filters={filters}
                toggleStation={toggleStation}
                toggleType={toggleType}
            />

            <main>
                {filteredMessages.length === 0 ? (
                    <div className="text-secondary text-center py-10">
                        {messages.length === 0 ? 'Waiting for messages...' : 'No messages match your filters.'}
                    </div>
                ) : (
                    filteredMessages.map(msg => (
                        <MessageCard key={msg.id} message={msg} />
                    ))
                )}
            </main>
        </div>
    );
};

export default Dashboard;
