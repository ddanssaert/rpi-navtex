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
    event.respondWith(
        caches.match(event.request).then((response) => {
            return response || fetch(event.request);
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
