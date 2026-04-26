import { useState, useEffect } from 'react';

const Settings = () => {
    const [config, setConfig] = useState({ antenna: 'A', lna_gain: 0 });
    const [saving, setSaving] = useState(false);
    const [msg, setMsg] = useState('');
    const [notifStatus, setNotifStatus] = useState(Notification.permission);

    useEffect(() => {
        fetch('/config')
            .then(res => res.json())
            .then(data => setConfig(data))
            .catch(err => console.error('Error fetching config:', err));
    }, []);

    const handleSave = () => {
        setSaving(true);
        fetch('/config', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(config)
        })
            .then(res => res.json())
            .then(() => {
                setMsg('Settings saved successfully!');
                setTimeout(() => setMsg(''), 3000);
            })
            .catch(err => setMsg('Error saving settings'))
            .finally(() => setSaving(false));
    };

    const requestNotifications = () => {
        Notification.requestPermission().then(permission => {
            setNotifStatus(permission);
        });
    };

    return (
        <div className="glass-card">
            <h2 className="mb-4">SDR Configuration</h2>

            <div className="mb-4">
                <label className="block text-secondary text-xs uppercase mb-1">Antenna Port</label>
                <div className="flex gap-2">
                    {['A', 'B', 'C'].map(p => (
                        <button
                            key={p}
                            onClick={() => setConfig(prev => ({ ...prev, antenna: p }))}
                            className={`filter-chip ${config.antenna === p ? 'active' : ''}`}
                        >
                            Port {p}
                        </button>
                    ))}
                </div>
            </div>

            <div className="mb-6">
                <label className="block text-secondary text-xs uppercase mb-1">LNA Gain State (0-9)</label>
                <input
                    type="range"
                    min="0" max="9"
                    value={config.lna_gain}
                    onChange={(e) => setConfig(prev => ({ ...prev, lna_gain: parseInt(e.target.value) }))}
                    className="w-full accent-emerald-400"
                />
                <div className="flex justify-between text-xs text-secondary mt-1">
                    <span>Minimum</span>
                    <span className="text-accent-color font-bold">Current: {config.lna_gain}</span>
                    <span>Maximum</span>
                </div>
            </div>

            <button
                onClick={handleSave}
                disabled={saving}
                className="save-button w-full mb-6"
            >
                {saving ? 'Saving...' : 'Apply Configuration'}
            </button>

            {msg && <div className="mt-4 text-center text-sm text-accent-color">{msg}</div>}

            <hr className="my-6 border-glass-border" />

            <h2 className="mb-4">Client Experience</h2>

            <div className="mb-4">
                <label className="block text-secondary text-xs uppercase mb-1">Push Notifications</label>
                <button
                    onClick={requestNotifications}
                    disabled={notifStatus === 'granted'}
                    className={`w-full p-3 rounded text-sm transition ${notifStatus === 'granted' ? 'bg-accent-color/20 text-accent-color' : 'bg-black/40 border border-glass-border hover:bg-black/60'}`}
                >
                    {notifStatus === 'granted' ? '✓ Notifications Enabled' : 'Enable Push Notifications'}
                </button>
            </div>

            <h2 className="mb-4 mt-6">System</h2>
            <button className="bg-black/40 border border-glass-border p-3 rounded w-full text-sm hover:bg-black/60 transition">
                Check for Software Updates
            </button>
        </div>
    );
};

export default Settings;
