import { readFileSync } from 'fs';
import { resolve } from 'path';
import { vi, describe, it, expect, beforeEach } from 'vitest';

const swSource = readFileSync(resolve(__dirname, '../../public/sw.js'), 'utf-8');

function createMockSelf() {
    const handlers = {};
    return {
        addEventListener: vi.fn((event, handler) => { handlers[event] = handler; }),
        clients: { matchAll: vi.fn() },
        registration: { showNotification: vi.fn().mockResolvedValue(undefined) },
        _handlers: handlers,
    };
}

function loadSW(mockSelf) {
    const mockCaches = { open: vi.fn().mockResolvedValue({ addAll: vi.fn() }) };
    new Function('self', 'caches', swSource)(mockSelf, mockCaches);
}

function makePushEvent(overrides = {}) {
    let capturedPromise;
    const event = {
        data: null,
        waitUntil: vi.fn(p => { capturedPromise = p; }),
        ...overrides,
    };
    return { event, getPromise: () => capturedPromise };
}

describe('sw.js push event handler', () => {
    let mockSelf;

    beforeEach(() => {
        mockSelf = createMockSelf();
        loadSW(mockSelf);
    });

    it('does not include badge in notification options when no window is focused', async () => {
        mockSelf.clients.matchAll.mockResolvedValue([]);
        const { event, getPromise } = makePushEvent();
        mockSelf._handlers.push(event);
        await getPromise();
        const [, options] = mockSelf.registration.showNotification.mock.calls[0];
        expect(options).not.toHaveProperty('badge');
    });

    it('does not include renotify in notification options when no window is focused', async () => {
        mockSelf.clients.matchAll.mockResolvedValue([]);
        const { event, getPromise } = makePushEvent();
        mockSelf._handlers.push(event);
        await getPromise();
        const [, options] = mockSelf.registration.showNotification.mock.calls[0];
        expect(options).not.toHaveProperty('renotify');
    });

    it('does NOT show notification when a window client is visible (focus-check guard)', async () => {
        mockSelf.clients.matchAll.mockResolvedValue([{ visibilityState: 'visible' }]);
        const { event, getPromise } = makePushEvent();
        mockSelf._handlers.push(event);
        await getPromise();
        expect(mockSelf.registration.showNotification).not.toHaveBeenCalled();
    });

    it('DOES show notification when no window client is visible', async () => {
        mockSelf.clients.matchAll.mockResolvedValue([]);
        const { event, getPromise } = makePushEvent();
        mockSelf._handlers.push(event);
        await getPromise();
        expect(mockSelf.registration.showNotification).toHaveBeenCalledOnce();
    });

    it('event.waitUntil receives a settled promise on the focused (skip) branch', async () => {
        mockSelf.clients.matchAll.mockResolvedValue([{ visibilityState: 'visible' }]);
        const { event, getPromise } = makePushEvent();
        mockSelf._handlers.push(event);
        expect(event.waitUntil).toHaveBeenCalledOnce();
        await expect(getPromise()).resolves.toBeUndefined();
        expect(mockSelf.registration.showNotification).not.toHaveBeenCalled();
    });
});
