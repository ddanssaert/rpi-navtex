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
    event.waitUntil(
        (async () => {
            // Default fallback if parsing fails completely
            let title = 'NAVTEX Alert';
            let options = {
                body: 'New maritime safety information available.',
                icon: '/icon-192.png',
                badge: '/favicon.svg',
                tag: 'navtex-msg',
                renotify: true,
                data: { url: '/' }
            };

            try {
                if (event.data) {
                    const data = event.data.json();

                    // Override defaults with payload data
                    title = (data.station_id && data.message_type)
                        ? `${data.station_id}${data.message_type}: New Message`
                        : data.title || title;

                    options.body = data.content || data.body || options.body;
                }
            } catch (error) {
                console.error('[SW] Payload parsing failed, falling back to default notification:', error);
                // We deliberately do not return here. We MUST proceed to render.
            }

            // Guarantee a render promise is returned to the OS
            return self.registration.showNotification(title, options);
        })()
    );
});

self.addEventListener('notificationclick', (event) => {
    event.notification.close();
    event.waitUntil(
        clients.openWindow(event.notification.data.url)
    );
});
