// NAVTEX code lookup table — European + Mediterranean focus.
// Source of truth for human-readable station and message-type labels.
// See ITU-R M.540 for the standard message-type assignments.

export const STATION_NAMES = {
    A: 'Niton (UK)',
    B: 'Cullercoats (UK)',
    C: 'Oostende (Belgium)',
    D: 'Netherlands Coastguard',
    E: 'Malin Head (Ireland)',
    F: 'Ship (unassigned)',
    G: 'Pinnacle (unassigned)',
    H: 'Noord-Nederland',
    I: 'IJmuiden (Netherlands)',
    J: 'Portpatrick (UK)',
    K: 'Cullercoats (UK, alt)',
    L: 'Niton (UK, alt)',
    M: 'Hamburg (Germany)',
    N: 'Oostende (Belgium, alt)',
    O: 'Portishead (UK)',
    P: 'Ionian (Greece)',
    Q: 'Corsen (France)',
    R: 'Roma (Italy)',
    S: 'Monsanto (Portugal)',
    T: 'Nicosia (Cyprus)',
    U: 'Kavala (Greece)',
    V: 'Kerkyra (Greece)',
    W: 'Bodo (Norway)',
    X: 'Murmansk (Russia)',
    Y: 'Archangel (Russia)',
    Z: 'Split / Other'
};

export const MESSAGE_TYPE_NAMES = {
    A: 'Navigation Warnings',
    B: 'Meteorological Warnings',
    C: 'Ice Reports',
    D: 'Search and Rescue',
    E: 'Meteorological Forecasts',
    F: 'Pilot Messages',
    G: 'AIS Messages',
    H: 'LORAN Messages',
    I: 'Omega Messages',
    J: 'SATNAV Messages',
    K: 'Other Electronic Navigational Aid Messages',
    L: 'Navigational Warnings (additional)',
    M: 'Unassigned (M)',
    N: 'Unassigned (N)',
    O: 'Unassigned (O)',
    P: 'Unassigned (P)',
    Q: 'Unassigned (Q)',
    R: 'Unassigned (R)',
    S: 'Unassigned (S)',
    T: 'Unassigned (T)',
    U: 'Unassigned (U)',
    V: 'Notice to Fishermen',
    W: 'Environmental',
    X: 'Special Services',
    Y: 'Special Services',
    Z: 'No Messages On Hand'
};

export const resolveStation = (code) => {
    const name = STATION_NAMES[code];
    return name ? `${code} — ${name}` : `Unknown (${code})`;
};

export const resolveType = (code) => {
    const name = MESSAGE_TYPE_NAMES[code];
    return name ? `${code} — ${name}` : `Unknown (${code})`;
};
