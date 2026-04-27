const DB_NAME = 'navtex-db';
const DB_VERSION = 1;
const STORE_NAME = 'settings';

export const initDB = () => {
    return new Promise((resolve, reject) => {
        const request = indexedDB.open(DB_NAME, DB_VERSION);
        request.onupgradeneeded = (event) => {
            const db = event.target.result;
            if (!db.objectStoreNames.contains(STORE_NAME)) {
                db.createObjectStore(STORE_NAME);
            }
        };
        request.onsuccess = (event) => {
            resolve(event.target.result);
        };
        request.onerror = (event) => {
            reject(event.target.error);
        };
    });
};

export const getFilters = async () => {
    const db = await initDB();
    return new Promise((resolve, reject) => {
        const transaction = db.transaction(STORE_NAME, 'readonly');
        const store = transaction.objectStore(STORE_NAME);
        const request = store.get('filters');
        request.onsuccess = () => {
            // Default: all stations A-Z, all types A-Z
            resolve(request.result || {
                stations: [], // Empty means all in our logic? Or should we prepopulate?
                types: []     // Let's decide empty means "ALL" or "NOTHING".
            });
        };
        request.onerror = () => {
            reject(request.error);
        };
    });
};

export const setFilters = async (filters) => {
    const db = await initDB();
    return new Promise((resolve, reject) => {
        const transaction = db.transaction(STORE_NAME, 'readwrite');
        const store = transaction.objectStore(STORE_NAME);
        const request = store.put(filters, 'filters');
        request.onsuccess = () => {
            resolve();
        };
        request.onerror = () => {
            reject(request.error);
        };
    });
};
