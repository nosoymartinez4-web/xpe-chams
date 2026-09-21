const getRawBody = require('raw-body');
const licenses = global.__licenses || {};
global.__licenses = licenses;

function genKey() {
    const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789';
    let k = 'XPE-';
    for (let i = 0; i < 8; i++) k += chars[Math.floor(Math.random() * chars.length)];
    k += '-';
    for (let i = 0; i < 4; i++) k += chars[Math.floor(Math.random() * chars.length)];
    return k;
}

module.exports = async (req, res) => {
    res.setHeader('Access-Control-Allow-Origin', '*');
    res.setHeader('Access-Control-Allow-Methods', 'POST, OPTIONS');
    res.setHeader('Access-Control-Allow-Headers', 'Content-Type');
    if (req.method === 'OPTIONS') { res.status(200).end(); return; }
    if (req.method !== 'POST') { return res.status(405).json({ success: false }); }

    try {
        const raw = await getRawBody(req);
        const data = JSON.parse(raw.toString());

        if (!data.token || data.token.length < 5) {
            return res.status(401).json({ success: false, message: 'No autorizado' });
        }

        switch (data.action) {
            case 'generate': {
                const days = parseInt(data.duration) || 30;
                const newKey = genKey();
                const created = new Date();
                const expiry = new Date(created);
                expiry.setDate(expiry.getDate() + days);
                licenses[newKey] = {
                    key: newKey, username: data.customer || '',
                    created: created.toISOString().split('T')[0],
                    expiry: expiry.toISOString().split('T')[0],
                    active: true, hwid: '', seller: data.username || 'unknown', lastLogin: ''
                };
                return res.json({ success: true, key: newKey, expiry: licenses[newKey].expiry });
            }
            case 'list': {
                const myList = Object.values(licenses).filter(l => l.seller === data.username);
                return res.json({ success: true, licenses: myList });
            }
            case 'stats': {
                const mine = Object.values(licenses).filter(l => l.seller === data.username);
                return res.json({ success: true, stats: {
                    total: mine.length,
                    active: mine.filter(l => l.active).length
                }});
            }
            default:
                return res.json({ success: false, message: 'Acción desconocida' });
        }
    } catch (e) {
        res.status(400).json({ success: false, message: 'Error: ' + e.message });
    }
};