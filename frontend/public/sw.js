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

    const options = {
        body: data.content || data.body,
        icon: '/icon-192.png',
        badge: '/favicon.svg',
        data: {
            url: '/'
        }
    };

    event.waitUntil(
        self.registration.showNotification(data.station_id + data.message_type + ': ' + data.title || 'NAVTEX Alert', options)
    );
});

self.addEventListener('notificationclick', (event) => {
    event.notification.close();
    event.waitUntil(
        clients.openWindow(event.notification.data.url)
    );
});
