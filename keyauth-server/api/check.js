// XPE KeyAuth - Check subscription status
const licenses = global.__licenses || {};
global.__licenses = licenses;

module.exports = async (req, res) => {
    res.setHeader('Access-Control-Allow-Origin', '*');
    res.setHeader('Access-Control-Allow-Methods', 'POST, OPTIONS');
    res.setHeader('Access-Control-Allow-Headers', 'Content-Type');
    if (req.method === 'OPTIONS') { res.status(200).end(); return; }
    if (req.method !== 'POST') { return res.status(405).json({ success: false }); }

    let body = '';
    await new Promise(resolve => { req.on('data', c => body += c); req.on('end', resolve); });

    try {
        const { key } = JSON.parse(body);
        if (!key) return res.status(400).json({ success: false, message: 'Key requerida' });

        const lic = licenses[key];
        if (!lic) return res.json({ success: false, message: 'Licencia inválida' });
        if (!lic.active) return res.json({ success: false, message: 'Licencia revocada' });

        const now = new Date();
        const expiry = new Date(lic.expiry);
        if (now > expiry) {
            lic.active = false;
            return res.json({ success: false, message: 'Licencia expirada' });
        }

        res.json({ success: true, username: lic.username || key, expiry: lic.expiry });
    } catch (e) {
        res.status(400).json({ success: false, message: 'Error: ' + e.message });
    }
};