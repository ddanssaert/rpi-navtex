import { useState, useEffect } from 'react';

const Settings = ({ lastNotifError }) => {
    const [config, setConfig] = useState({ antenna: 'A', lna_gain: 0, bias_t: false });
    const [saving, setSaving] = useState(false);
    const [msg, setMsg] = useState('');
    const [notifStatus, setNotifStatus] = useState(
        'Notification' in window ? Notification.permission : 'unsupported'
    );

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
        if (!('Notification' in window)) {
            setMsg('Notifications are not supported in this browser or context (HTTPS may be required).');
            return;
        }
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

            <div className="flex items-center justify-between p-3 bg-black/20 rounded-lg border border-glass-border mb-6">
                <div>
                    <div className="text-sm font-medium">Bias-T Power</div>
                    <div className="text-[10px] text-secondary uppercase">Power for active antenna</div>
                </div>
                <button
                    onClick={() => setConfig(prev => ({ ...prev, bias_t: !prev.bias_t }))}
                    className={`w-10 h-5 rounded-full transition-colors relative ${config.bias_t ? 'bg-emerald-500' : 'bg-white/10'}`}
                >
                    <div className={`absolute top-1 w-3 h-3 bg-white rounded-full transition-transform ${config.bias_t ? 'translate-x-6' : 'translate-x-1'}`} />
                </button>
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

            <div className="mb-4">
                <button
                    onClick={() => {
                        fetch('/test-notify', { method: 'POST' })
                            .then(res => res.json())
                            .then(data => {
                                setMsg('Broadcast triggered! Check other clients.');
                                setTimeout(() => setMsg(''), 3000);
                            })
                            .catch(err => setMsg('Error triggering broadcast'));
                    }}
                    className="w-full p-3 rounded text-sm bg-black/40 border border-glass-border hover:bg-emerald-500/10 hover:border-emerald-500/50 transition flex items-center justify-center gap-2 mb-2"
                >
                    <span>🌐</span> Broadcast Test (on all clients)
                </button>

                <button
                    onClick={() => {
                        if ('serviceWorker' in navigator && 'Notification' in window) {
                            navigator.serviceWorker.ready.then(reg => {
                                reg.showNotification('NAVTEX Local Test', {
                                    body: 'This notification was triggered directly by your click.',
                                    icon: '/icon-192.png'
                                });
                                setMsg('Local notification triggered!');
                            }).catch(err => setMsg('Local notify failed: ' + err.message));
                        } else {
                            setMsg('Notifications not supported in this browser.');
                        }
                    }}
                    className="w-full p-3 rounded text-sm bg-black/40 border border-glass-border hover:bg-emerald-500/10 hover:border-emerald-500/50 transition flex items-center justify-center gap-2"
                >
                    <span>�</span> Local Notification Test (this device only)
                </button>
            </div>

            <h2 className="mb-4 mt-6">Privacy & Security</h2>
            <div className="bg-black/40 border border-glass-border p-4 rounded text-sm space-y-4">
                <p className="text-secondary text-xs">
                    To enable native PWA features and Push Notifications on your local network, you must download and trust the Root CA certificate generated by this device.
                </p>
                <a
                    href="/certs/ca.crt"
                    download="navtex-root-ca.crt"
                    className="flex items-center justify-center gap-2 w-full p-3 bg-emerald-500 text-white rounded hover:bg-emerald-400 transition font-bold no-underline"
                >
                    <span>🛡️</span> Download Root CA Certificate
                </a>
                <div className="text-[10px] text-secondary border-t border-glass-border pt-2 space-y-2">
                    <div className="flex gap-2">
                        <span className="font-bold text-accent-color">Android:</span>
                        <span>Settings → Security → Install from storage → CA Certificate.</span>
                    </div>
                    <div className="flex gap-2">
                        <span className="font-bold text-accent-color">iOS:</span>
                        <span>Download profile → Settings → General → VPN & Device Management → Install → About → Certificate Trust Settings → Enable.</span>
                    </div>
                </div>
            </div>

            <h2 className="mb-4 mt-6">System Diagnostic</h2>
            <div className="bg-black/40 border border-glass-border p-4 rounded text-xs font-mono space-y-2">
                <div className="flex justify-between">
                    <span className="text-secondary">Secure Context:</span>
                    <span className={window.isSecureContext ? 'text-emerald-400' : 'text-rose-400'}>
                        {window.isSecureContext ? 'YES' : 'NO (Required for Push)'}
                    </span>
                </div>
                <div className="flex justify-between">
                    <span className="text-secondary">Notification API:</span>
                    <span className={'Notification' in window ? 'text-emerald-400' : 'text-rose-400'}>
                        {'Notification' in window ? 'Available' : 'Missing'}
                    </span>
                </div>
                <div className="flex justify-between">
                    <span className="text-secondary">Current Permission:</span>
                    <span className="text-accent-color">{notifStatus}</span>
                </div>
                <div className="flex justify-between">
                    <span className="text-secondary">PWA Mode:</span>
                    <span className={window.matchMedia('(display-mode: standalone)').matches ? 'text-emerald-400' : 'text-rose-400'}>
                        {window.matchMedia('(display-mode: standalone)').matches ? 'Standalone (Home Screen)' : 'Browser Tab'}
                    </span>
                </div>
                {lastNotifError && (
                    <div className="mt-2 text-[10px] text-rose-400 border-t border-glass-border pt-1">
                        Last Failure: {lastNotifError}
                    </div>
                )}
                {!window.isSecureContext && (
                    <div className="mt-4 p-2 bg-rose-500/20 border border-rose-500/50 rounded text-rose-200">
                        ⚠️ Most mobile browsers disable Notifications over HTTP. Use HTTPS or localhost.
                    </div>
                )}
            </div>

            <h2 className="mb-4 mt-6">System</h2>
            <button className="bg-black/40 border border-glass-border p-3 rounded w-full text-sm hover:bg-black/60 transition">
                Check for Software Updates
            </button>
        </div >
    );
};

export default Settings;
