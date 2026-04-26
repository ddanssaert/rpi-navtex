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
    const [notifError, setNotifError] = useState('');
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
                if (newMsg.type === 'test_notification') {
                    if ('Notification' in window && Notification.permission === 'granted') {
                        const triggerNotif = (reg) => {
                            if (!reg) return;
                            reg.showNotification(newMsg.title, {
                                body: newMsg.body,
                                icon: '/icon-192.png',
                                badge: '/favicon.svg'
                            }).catch(err => {
                                setNotifError('Notification failed: ' + err.message);
                            });
                        };

                        if ('serviceWorker' in navigator) {
                            navigator.serviceWorker.getRegistration().then(reg => {
                                if (reg) triggerNotif(reg);
                                else navigator.serviceWorker.ready.then(triggerNotif);
                            }).catch(err => setNotifError('SW registration error: ' + err.message));
                        } else {
                            try {
                                new Notification(newMsg.title, { body: newMsg.body });
                            } catch (err) {
                                setNotifError('Direct notification error: ' + err.message);
                            }
                        }
                    } else {
                        setNotifError('Notifications not allowed or insecure context.');
                    }
                } else {
                    setMessages(prev => [newMsg, ...prev]);

                    // Show push notification for real messages if not on dashboard
                    if ('Notification' in window && Notification.permission === 'granted' && activeTab !== 'messages') {
                        navigator.serviceWorker.getRegistration().then(reg => {
                            if (reg) {
                                reg.showNotification(`New NAVTEX: ${newMsg.station_id}`, {
                                    body: newMsg.content.substring(0, 100) + '...',
                                    icon: '/icon-192.png',
                                    badge: '/favicon.svg',
                                    tag: 'new-message'
                                });
                            }
                        });
                    }
                }
            },
            (online) => setIsOnline(online)
        );

        return cleanup;
    }, [activeTab]);

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
                <Settings lastNotifError={notifError} />
            )}
        </div>
    );
};

export default Dashboard;
