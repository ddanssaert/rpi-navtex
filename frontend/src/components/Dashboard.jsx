import { useState, useEffect } from 'react';
import MessageCard from './MessageCard';
import FilterBar from './FilterBar';
import Settings from './Settings';
import useFilters from '../hooks/useFilters';
import { createWebSocket } from '../api/websocket';

const Dashboard = () => {
    const [activeTab, setActiveTab] = useState('messages');
    const [messages, setMessages] = useState([]);
    const [isOnline, setIsOnline] = useState(false);
    const { filters, toggleStation, toggleType, isFiltered } = useFilters();

    useEffect(() => {
        // Load history
        fetch('/messages')
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

            <nav className="nav-tabs">
                <button
                    className={`nav-tab ${activeTab === 'messages' ? 'active' : ''}`}
                    onClick={() => setActiveTab('messages')}
                >
                    MESSAGES
                </button>
                <button
                    className={`nav-tab ${activeTab === 'settings' ? 'active' : ''}`}
                    onClick={() => setActiveTab('settings')}
                >
                    SETTINGS
                </button>
            </nav>

            {activeTab === 'messages' ? (
                <>
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
                </>
            ) : (
                <Settings />
            )}
        </div>
    );
};

export default Dashboard;
