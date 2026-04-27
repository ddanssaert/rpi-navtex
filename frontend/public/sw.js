const CACHE_NAME = 'navtex-v1';
const ASSETS = [
    '/',
    '/index.html',
    '/manifest.json',
    '/icon-192.png',
    '/icon-512.png',
    '/icon-maskable-192.png',
    '/icon-maskable-512.png'
];

self.addEventListener('install', (event) => {
    event.waitUntil(
        caches.open(CACHE_NAME).then((cache) => {
            return cache.addAll(ASSETS);
        })
    );
});

self.addEventListener('fetch', (event) => {
    // Only intercept GET requests
    if (event.request.method !== 'GET') return;

    event.respondWith(
        caches.match(event.request).then((response) => {
            return response || fetch(event.request).catch((err) => {
                console.error('[SW] Fetch failed:', event.request.url, err);
                // Return a generic error response instead of letting the promise reject
                // this prevents the white-screen error in Safari when certs are untrusted.
                return new Response('Network error or untrusted certificate', {
                    status: 503,
                    statusText: 'Service Unavailable',
                    headers: new Headers({ 'Content-Type': 'text/plain' })
                });
            });
        })
    );
});

self.addEventListener('push', (event) => {
    const data = event.data ? event.data.json() : { title: 'New NAVTEX Message', body: 'Check your dashboard.' };

    const showNotification = async () => {
        // 1. Check if any client is focused
        const clientList = await clients.matchAll({ type: 'window', includeUncontrolled: true });
        const isFocused = clientList.some(client => client.focused);
        if (isFocused) {
            console.log('[SW] App is focused, suppressing notification');
            return;
        }

        // 2. Read filters from IndexedDB
        // Since we can't easily import a module in a public sw.js without bundling,
        // we use a simple inline implementation.
        let filters = { stations: [], types: [] };
        try {
            filters = await new Promise((resolve, reject) => {
                const request = indexedDB.open('navtex-db', 1);
                request.onerror = () => resolve({ stations: [], types: [] });
                request.onsuccess = (e) => {
                    const db = e.target.result;
                    if (!db.objectStoreNames.contains('settings')) {
                        resolve({ stations: [], types: [] });
                        return;
                    }
                    const tx = db.transaction('settings', 'readonly');
                    const store = tx.objectStore('settings');
                    const getReq = store.get('filters');
                    getReq.onsuccess = () => resolve(getReq.result || { stations: [], types: [] });
                    getReq.onerror = () => resolve({ stations: [], types: [] });
                };
            });
        } catch (e) {
            console.error('[SW] Failed to read filters:', e);
        }

        // 3. Apply filters
        // If filters are empty, it means "NOTHING" is filtered (show all)
        // No, wait, if users want specific stations, they'll list them.
        // Let's decide: if stations list is NOT empty, then data.station_id MUST be in it.
        // Same for types.
        const stationFilter = filters.stations || [];
        const typeFilter = filters.types || [];

        const stationMatch = stationFilter.length === 0 || stationFilter.includes(data.station_id);
        const typeMatch = typeFilter.length === 0 || typeFilter.includes(data.message_type);

        if (!stationMatch || !typeMatch) {
            console.log('[SW] Notification filtered out by user preference');
            return;
        }

        const options = {
            body: data.content || data.body,
            icon: '/icon-192.png',
            badge: '/favicon.svg',
            tag: 'navtex-msg', // Consolidate multiple notifications
            renotify: true,
            data: {
                url: '/'
            }
        };

        const title = (data.station_id && data.message_type)
            ? `${data.station_id}${data.message_type}: New Message`
            : data.title || 'NAVTEX Alert';

        await self.registration.showNotification(title, options);
    };

    event.waitUntil(showNotification());
});

self.addEventListener('notificationclick', (event) => {
    event.notification.close();
    event.waitUntil(
        clients.openWindow(event.notification.data.url)
    );
});
