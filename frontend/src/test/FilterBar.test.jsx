import { render, screen, fireEvent } from '@testing-library/react';
import { vi, describe, it, expect } from 'vitest';
import FilterBar from '../components/FilterBar';

const defaultFilters = { stations: [], types: [] };
const twoActiveFilters = { stations: ['A', 'B'], types: [] };

describe('FilterBar', () => {
    it('both sections are collapsed by default', () => {
        render(<FilterBar filters={defaultFilters} toggleStation={vi.fn()} toggleType={vi.fn()} />);
        expect(screen.queryByRole('checkbox')).not.toBeInTheDocument();
    });

    it('shows "Stations (0 active)" when no stations active', () => {
        render(<FilterBar filters={defaultFilters} toggleStation={vi.fn()} toggleType={vi.fn()} />);
        expect(screen.getByText(/Stations \(0 active\)/i)).toBeInTheDocument();
    });

    it('shows "Stations (2 active)" when two stations active', () => {
        render(<FilterBar filters={twoActiveFilters} toggleStation={vi.fn()} toggleType={vi.fn()} />);
        expect(screen.getByText(/Stations \(2 active\)/i)).toBeInTheDocument();
    });

    it('clicking Stations header expands and shows 26 checkboxes', () => {
        render(<FilterBar filters={defaultFilters} toggleStation={vi.fn()} toggleType={vi.fn()} />);
        fireEvent.click(screen.getByText(/Stations \(0 active\)/i));
        const checkboxes = screen.getAllByRole('checkbox');
        expect(checkboxes).toHaveLength(26);
    });

    it('clicking a station checkbox calls toggleStation with its code', () => {
        const toggleStation = vi.fn();
        render(<FilterBar filters={defaultFilters} toggleStation={toggleStation} toggleType={vi.fn()} />);
        fireEvent.click(screen.getByText(/Stations \(0 active\)/i));
        fireEvent.click(screen.getByLabelText('A — Niton (UK)'));
        expect(toggleStation).toHaveBeenCalledWith('A');
    });

    it('clicking a type checkbox calls toggleType with its code', () => {
        const toggleType = vi.fn();
        render(<FilterBar filters={defaultFilters} toggleStation={vi.fn()} toggleType={toggleType} />);
        fireEvent.click(screen.getByText(/Message Types \(0 active\)/i));
        fireEvent.click(screen.getByLabelText('A — Navigation Warnings'));
        expect(toggleType).toHaveBeenCalledWith('A');
    });
});
