import { render, waitFor, act } from '@testing-library/react';
import { vi, describe, it, expect, beforeEach, afterEach } from 'vitest';
import Settings from '../components/Settings';

function makeMockNotification(permission) {
    const MockNotification = function (title, options) {
        this.title = title;
        this.options = options;
    };
    MockNotification.permission = permission;
    MockNotification.requestPermission = vi.fn().mockResolvedValue(permission);
    return MockNotification;
}

function makeMockServiceWorker({ subscription }) {
    return {
        ready: Promise.resolve({
            pushManager: {
                getSubscription: () => Promise.resolve(subscription),
                subscribe: vi.fn().mockResolvedValue({
                    endpoint: 'https://push.example.com/sub',
                    toJSON: () => ({ endpoint: 'https://push.example.com/sub', keys: {} }),
                }),
            },
        }),
    };
}

beforeEach(() => {
    global.fetch = vi.fn().mockImplementation((url) => {
        if (url === '/push/vapid-key') {
            return Promise.resolve({ json: () => Promise.resolve({ public_key: 'test-vapid-key' }) });
        }
        if (url === '/push/subscribe') {
            return Promise.resolve({ ok: true, json: () => Promise.resolve({}) });
        }
        if (url === '/config') {
            return Promise.resolve({ json: () => Promise.resolve({ antenna: 'A', lna_gain: 0, bias_t: false }) });
        }
        return Promise.resolve({ json: () => Promise.resolve({}) });
    });
});

afterEach(() => {
    vi.restoreAllMocks();
});

describe('Settings silent re-subscription on mount', () => {
    it('calls handlePushSubscription when permission is granted and subscription is missing', async () => {
        global.Notification = makeMockNotification('granted');
        Object.defineProperty(global.navigator, 'serviceWorker', {
            value: makeMockServiceWorker({ subscription: null }),
            configurable: true,
        });

        render(<Settings />);

        await waitFor(() => {
            expect(global.fetch).toHaveBeenCalledWith('/push/vapid-key');
        });
    });

    it('does NOT call handlePushSubscription when permission is default and subscription is missing', async () => {
        global.Notification = makeMockNotification('default');
        Object.defineProperty(global.navigator, 'serviceWorker', {
            value: makeMockServiceWorker({ subscription: null }),
            configurable: true,
        });

        await act(async () => {
            render(<Settings />);
            await new Promise(r => setTimeout(r, 100));
        });

        expect(global.fetch).not.toHaveBeenCalledWith('/push/vapid-key');
    });

    it('does NOT call handlePushSubscription when subscription already exists', async () => {
        global.Notification = makeMockNotification('granted');
        Object.defineProperty(global.navigator, 'serviceWorker', {
            value: makeMockServiceWorker({ subscription: { endpoint: 'https://push.example.com/existing' } }),
            configurable: true,
        });

        await act(async () => {
            render(<Settings />);
            await new Promise(r => setTimeout(r, 100));
        });

        expect(global.fetch).not.toHaveBeenCalledWith('/push/vapid-key');
    });
});
