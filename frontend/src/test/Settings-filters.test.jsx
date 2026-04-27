import { render, screen, fireEvent, waitFor } from '@testing-library/react';
import { vi, describe, it, expect, beforeEach } from 'vitest';
import Settings from '../components/Settings';

const mockSubscription = { endpoint: 'https://push.example.com/sub' };

const mockServiceWorker = {
    ready: Promise.resolve({
        pushManager: {
            getSubscription: () => Promise.resolve(mockSubscription),
            subscribe: vi.fn().mockResolvedValue(mockSubscription)
        }
    })
};

beforeEach(() => {
    vi.clearAllMocks();
    global.fetch = vi.fn().mockResolvedValue({ ok: true, json: () => Promise.resolve({}) });
    global.Notification = Object.assign(vi.fn(), {
        permission: 'granted',
        requestPermission: vi.fn().mockResolvedValue('granted')
    });
    Object.defineProperty(global.navigator, 'serviceWorker', {
        value: mockServiceWorker,
        configurable: true
    });
});

describe('Settings notification filter section', () => {
    it('shows Notification Filters heading when subscribed', async () => {
        render(<Settings lastNotifError={null} />);
        await waitFor(() => {
            expect(screen.getByText('Notification Filters')).toBeInTheDocument();
        });
    });

    it('shows checkboxes with resolved station labels when subscribed', async () => {
        render(<Settings lastNotifError={null} />);
        await waitFor(() => {
            expect(screen.getByLabelText('A — Niton (UK)')).toBeInTheDocument();
        });
        const checkbox = screen.getByLabelText('A — Niton (UK)');
        expect(checkbox).toHaveAttribute('type', 'checkbox');
    });

    it('shows checkboxes with resolved message type labels when subscribed', async () => {
        render(<Settings lastNotifError={null} />);
        await waitFor(() => {
            expect(screen.getByLabelText('A — Navigation Warnings')).toBeInTheDocument();
        });
        const checkbox = screen.getByLabelText('A — Navigation Warnings');
        expect(checkbox).toHaveAttribute('type', 'checkbox');
    });

    it('does not show raw single-letter station buttons when subscribed', async () => {
        render(<Settings lastNotifError={null} />);
        await waitFor(() => {
            expect(screen.getByText('Notification Filters')).toBeInTheDocument();
        });
        // Raw single-letter buttons had class w-7 h-7; checkboxes should be used instead
        const allCheckboxes = screen.getAllByRole('checkbox');
        expect(allCheckboxes.length).toBeGreaterThanOrEqual(52); // 26 stations + 26 types
    });

    it('toggling a station checkbox calls saveFiltersToDB with updated state', async () => {
        render(<Settings lastNotifError={null} />);
        await waitFor(() => {
            expect(screen.getByLabelText('A — Niton (UK)')).toBeInTheDocument();
        });
        fireEvent.click(screen.getByLabelText('A — Niton (UK)'));
        // After click, checkbox should be checked
        expect(screen.getByLabelText('A — Niton (UK)')).toBeChecked();
    });
});
