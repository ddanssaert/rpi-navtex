import { describe, it, expect } from 'vitest';
import { STATION_NAMES, MESSAGE_TYPE_NAMES, resolveStation, resolveType } from '../constants/navtex-codes';

describe('navtex-codes lookup', () => {
    it('STATION_NAMES has exactly 26 entries', () => {
        expect(Object.keys(STATION_NAMES)).toHaveLength(26);
    });

    it('MESSAGE_TYPE_NAMES has exactly 26 entries', () => {
        expect(Object.keys(MESSAGE_TYPE_NAMES)).toHaveLength(26);
    });

    it('resolveStation returns label with em-dash for known code', () => {
        expect(resolveStation('A')).toBe('A — Svalbard (Norway)');
    });

    it('national 490 kHz station overrides international for B', () => {
        expect(resolveStation('B')).toBe('B — Oostende (Belgium)');
    });

    it('national 490 kHz station overrides international for I', () => {
        expect(resolveStation('I')).toBe('I — Niton (UK)');
    });

    it('resolveStation returns Unknown format for unrecognised code', () => {
        expect(resolveStation('1')).toBe('Unknown (1)');
    });

    it('resolveType returns label with em-dash for known code', () => {
        expect(resolveType('A')).toBe('A — Navigation Warnings');
    });

    it('resolveType returns Unknown format for unrecognised code', () => {
        expect(resolveType('1')).toBe('Unknown (1)');
    });
});
