// XPE KeyAuth - Admin endpoints
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
    if (req.method !== 'POST') { return res.status(405).json({ success: false, message: 'Method not allowed' }); }

    const data = req.body || {};
    if (!data.token || data.token.length < 5) {
        return res.status(401).json({ success: false, message: 'No autorizado' });
    }

    switch (data.action) {
        case 'list': {
            const list = Object.values(licenses).map(l => ({
                key: l.key, username: l.username, created: l.created,
                expiry: l.expiry, active: l.active, hwid: l.hwid || '',
                seller: l.seller || '', lastLogin: l.lastLogin || ''
            }));
            return res.json({ success: true, licenses: list });
        }
        case 'generate': {
            const newKey = genKey();
            const days = parseInt(data.duration) || 365;
            const created = new Date();
            const expiry = new Date(created);
            expiry.setDate(expiry.getDate() + days);

            licenses[newKey] = {
                key: newKey, username: data.customer || '',
                created: created.toISOString().split('T')[0],
                expiry: expiry.toISOString().split('T')[0],
                active: true, hwid: '', seller: data.username || 'admin', lastLogin: ''
            };
            return res.json({ success: true, key: newKey, expiry: licenses[newKey].expiry });
        }
        case 'revoke': {
            if (!data.key || !licenses[data.key])
                return res.json({ success: false, message: 'No encontrada' });
            licenses[data.key].active = false;
            return res.json({ success: true, message: 'Revocada' });
        }
        case 'reactivate': {
            if (!data.key || !licenses[data.key])
                return res.json({ success: false, message: 'No encontrada' });
            licenses[data.key].active = true;
            return res.json({ success: true, message: 'Reactivada' });
        }
        case 'delete': {
            if (!data.key || !licenses[data.key])
                return res.json({ success: false, message: 'No encontrada' });
            delete licenses[data.key];
            return res.json({ success: true, message: 'Eliminada' });
        }
        case 'stats': {
            const all = Object.values(licenses);
            const total = all.length;
            const active = all.filter(l => l.active).length;
            const expired = all.filter(l => new Date(l.expiry) < new Date()).length;
            return res.json({ success: true, stats: { total, active, expired, revoked: total - active } });
        }
        default:
            return res.json({ success: false, message: 'Acción desconocida' });
    }
};