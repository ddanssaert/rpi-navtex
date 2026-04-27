import { describe, it, expect, beforeEach } from 'vitest';
import { getFilters, setFilters } from '../utils/db';

describe('IndexedDB utility', () => {
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
