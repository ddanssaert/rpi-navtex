import { describe, it, expect, beforeEach } from 'vitest';
import { getFilters, setFilters } from '../utils/db';

describe('IndexedDB utility', () => {
    beforeEach(async () => {
        const { setFilters } = await import('../utils/db');
        await setFilters({ stations: [], types: [] });
    });

    it('should initialize and store filters', async () => {
        const filters = {
            stations: ['A', 'B'],
            types: ['D', 'E']
        };
        await setFilters(filters);
        const saved = await getFilters();
        expect(saved).toEqual(filters);
    });

    it('should return default filters if none saved', async () => {
        const saved = await getFilters();
        expect(saved).toBeDefined();
        expect(saved.stations).toEqual([]);
    });
});
