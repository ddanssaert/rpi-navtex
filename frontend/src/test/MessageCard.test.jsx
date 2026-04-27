import { render, screen } from '@testing-library/react';
import { describe, it, expect } from 'vitest';
import MessageCard from '../components/MessageCard';

const message = {
    timestamp: '2024-01-01T00:00:00Z',
    station_id: 'A',
    message_type: 'B',
    serial_id: '001',
    content: 'TEST'
};

describe('MessageCard', () => {
    it('renders resolved station label not raw code', () => {
        render(<MessageCard message={message} />);
        expect(screen.getByText('A — Niton (UK)')).toBeInTheDocument();
        expect(screen.queryByText(/^A$/)).not.toBeInTheDocument();
    });

    it('renders resolved message type label not raw code', () => {
        render(<MessageCard message={message} />);
        expect(screen.getByText('B — Meteorological Warnings')).toBeInTheDocument();
    });

    it('falls back to Unknown format for unrecognised station code', () => {
        render(<MessageCard message={{ ...message, station_id: '?' }} />);
        expect(screen.getByText('Unknown (?)')).toBeInTheDocument();
    });
});
