import { renderHook, act } from '@testing-library/react';
import { vi, describe, it, expect, beforeEach, afterEach } from 'vitest';
import useFilters from '../hooks/useFilters';

let mockGetSubscription;

beforeEach(() => {
    vi.useFakeTimers();
    global.fetch = vi.fn().mockResolvedValue({ ok: true });
    mockGetSubscription = vi.fn().mockResolvedValue({ endpoint: 'https://push.example.com/sub' });
    Object.defineProperty(global.navigator, 'serviceWorker', {
        value: {
            ready: Promise.resolve({
                pushManager: { getSubscription: mockGetSubscription }
            })
        },
        configurable: true,
    });
    localStorage.clear();
});

afterEach(() => {
    vi.useRealTimers();
    vi.restoreAllMocks();
});

describe('useFilters server sync', () => {
    it('does NOT sync to server on initial mount', async () => {
        renderHook(() => useFilters());

        await act(async () => {
            vi.advanceTimersByTime(800);
            await Promise.resolve();
        });

        expect(global.fetch).not.toHaveBeenCalledWith('/push/filters', expect.anything());
    });

    it('syncs filters to server via PUT /push/filters after 800ms debounce on toggle', async () => {
        const { result } = renderHook(() => useFilters());

        act(() => {
            result.current.toggleStation('A');
        });

        // Not synced yet (within debounce window)
        expect(global.fetch).not.toHaveBeenCalledWith('/push/filters', expect.anything());

        await act(async () => {
            vi.advanceTimersByTime(800);
            await Promise.resolve();
            await Promise.resolve(); // flush async chain in syncFiltersToServer
        });

        expect(global.fetch).toHaveBeenCalledWith(
            '/push/filters',
            expect.objectContaining({
                method: 'PUT',
                headers: { 'Content-Type': 'application/json' },
            })
        );

        const body = JSON.parse(global.fetch.mock.calls.find(c => c[0] === '/push/filters')[1].body);
        expect(body.endpoint).toBe('https://push.example.com/sub');
        expect(body.filters.stations).toContain('A');
    });

    it('debounces: only fires once when toggled multiple times within 800ms', async () => {
        const { result } = renderHook(() => useFilters());

        act(() => { result.current.toggleStation('A'); });
        act(() => { result.current.toggleStation('B'); });
        act(() => { result.current.toggleStation('C'); });

        await act(async () => {
            vi.advanceTimersByTime(800);
            await Promise.resolve();
            await Promise.resolve();
        });

        const syncCalls = global.fetch.mock.calls.filter(c => c[0] === '/push/filters');
        expect(syncCalls.length).toBe(1);
    });

    it('does NOT sync when there is no active push subscription', async () => {
        mockGetSubscription.mockResolvedValue(null);
        const { result } = renderHook(() => useFilters());

        act(() => {
            result.current.toggleStation('A');
        });

        await act(async () => {
            vi.advanceTimersByTime(800);
            await Promise.resolve();
            await Promise.resolve();
        });

        expect(global.fetch).not.toHaveBeenCalledWith('/push/filters', expect.anything());
    });

    it('does NOT sync when serviceWorker is unavailable', async () => {
        Object.defineProperty(global.navigator, 'serviceWorker', {
            value: undefined,
            configurable: true,
        });
        const { result } = renderHook(() => useFilters());

        act(() => {
            result.current.toggleStation('A');
        });

        await act(async () => {
            vi.advanceTimersByTime(800);
            await Promise.resolve();
        });

        expect(global.fetch).not.toHaveBeenCalledWith('/push/filters', expect.anything());
    });
});
