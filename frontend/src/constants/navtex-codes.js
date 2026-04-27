// NAVTEX station lookup — Navarea 1 (North Atlantic, North Sea, Baltic Sea).
// 518 kHz international assignments, overridden by 490 kHz national where assigned.
// Source: ITU-R M.540 / Wikipedia Navarea 1 station list.

export const STATION_NAMES = {
    A: 'Svalbard (Norway)',          // 518 kHz
    B: 'Oostende (Belgium)',         // 490 kHz overrides Bodø
    C: 'Portpatrick (UK)',           // 490 kHz overrides Vardø
    D: 'Tórshavn (Faroe Islands)',   // 518 kHz
    E: 'Saudanes (Iceland)',         // 490 kHz overrides Niton
    F: 'Tallinn (Estonia)',          // 518 kHz
    G: 'Cullercoats (UK)',           // 518 kHz
    H: 'Bjuröklubb (Sweden)',        // 518 kHz
    I: 'Niton (UK)',                 // 490 kHz overrides Grimeton
    J: 'Gislövshammar (Sweden)',     // 518 kHz
    K: 'Grindavik (Iceland)',        // 490 kHz overrides Niton alt
    L: 'Pinneberg (Germany)',        // 490 kHz overrides Rogaland
    M: 'Jeløy (Norway)',             // 518 kHz
    N: 'Ørlandet (Norway)',          // 518 kHz
    O: 'Portpatrick (UK)',           // 518 kHz
    P: 'Netherlands Coastguard',     // 518 kHz
    Q: 'Malin Head (Ireland)',       // 518 kHz
    R: 'Saudanes (Iceland)',         // 518 kHz
    S: 'Pinneberg (Germany)',        // 518 kHz
    T: 'Oostende (Belgium)',         // 518 kHz
    U: 'Cullercoats (UK)',           // 490 kHz only
    V: 'Oostende (Belgium)',         // 518 kHz
    W: 'Valentia (Ireland)',         // 518 kHz
    X: 'Grindavik (Iceland)',        // 518 kHz
    Y: 'Unassigned (Navarea 1)',
    Z: 'Unassigned (Navarea 1)'
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
