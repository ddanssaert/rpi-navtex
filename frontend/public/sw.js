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
    if (event.request.method !== 'GET') return;

    const url = new URL(event.request.url);

    // Define paths that must strictly bypass the Service Worker cache.
    // Using startsWith allows for sub-routes like /messages/123 or /config/user
    const dynamicRoutes = ['/api/', '/messages', '/config'];

    const requiresNetwork = dynamicRoutes.some(route => url.pathname.startsWith(route));

    if (requiresNetwork) {
        // Return early to let the browser handle the network request natively
        return;
    }

    // Cache-First fallback for all other GET requests (static assets)
    event.respondWith(
        caches.match(event.request).then((response) => {
            return response || fetch(event.request).catch((err) => {
                console.error('[SW] Fetch failed:', event.request.url, err);
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
                tag: 'navtex-msg',
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

            const windowClients = await self.clients.matchAll({ includeUncontrolled: true, type: 'window' });
            const focused = windowClients.some(c => c.visibilityState === 'visible');
            if (!focused) {
                return self.registration.showNotification(title, options);
            }
            return Promise.resolve();
        })()
    );
});

self.addEventListener('notificationclick', (event) => {
    event.notification.close();
    event.waitUntil(
        clients.openWindow(event.notification.data.url)
    );
});
