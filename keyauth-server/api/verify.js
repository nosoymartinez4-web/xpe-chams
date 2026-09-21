// XPE KeyAuth - Verify license key (zero dependencies)
// In-memory storage
const licenses = global.__licenses || {};
global.__licenses = licenses;

// Seed demo keys
if (Object.keys(licenses).length === 0) {
    const d = new Date(); d.setFullYear(d.getFullYear() + 1);
    licenses['XPE-DEMO-2024-ABCD'] = {
        key: 'XPE-DEMO-2024-ABCD', username: 'demo_user',
        created: '2024-01-15', expiry: d.toISOString().split('T')[0],
        active: true, hwid: '', seller: 'xpe.nettt', lastLogin: ''
    };
}

module.exports = async (req, res) => {
    res.setHeader('Access-Control-Allow-Origin', '*');
    res.setHeader('Access-Control-Allow-Methods', 'POST, OPTIONS');
    res.setHeader('Access-Control-Allow-Headers', 'Content-Type');
    if (req.method === 'OPTIONS') { res.status(200).end(); return; }
    if (req.method !== 'POST') { return res.status(405).end(); }

    let body = '';
    await new Promise(resolve => { req.on('data', c => body += c); req.on('end', resolve); });

    try {
        const { key, hwid } = JSON.parse(body);
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

        // Bind HWID on first use
        if (!lic.hwid && hwid) lic.hwid = hwid;
        if (lic.hwid && hwid && lic.hwid !== hwid) {
            return res.json({ success: false, message: 'Licencia en uso en otro PC' });
        }

        lic.lastLogin = now.toISOString();

        // Calculate remaining time
        const diffMs = expiry - now;
        const diffHours = Math.floor(diffMs / (1000 * 60 * 60));
        const diffDays = Math.floor(diffMs / (1000 * 60 * 60 * 24));

        res.json({
            success: true,
            username: lic.username || key,
            expiry: lic.expiry,
            key: lic.key,
            remaining: diffDays > 0 ? diffDays + ' días' : diffHours > 0 ? diffHours + ' horas' : '< 1 hora'
        });
    } catch (e) {
        res.status(400).json({ success: false, message: 'JSON inválido' });
    }
};