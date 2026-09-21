// XPE KeyAuth - Auth endpoint (zero dependencies)
module.exports = async (req, res) => {
    res.setHeader('Access-Control-Allow-Origin', '*');
    res.setHeader('Access-Control-Allow-Methods', 'POST, OPTIONS');
    res.setHeader('Access-Control-Allow-Headers', 'Content-Type');
    if (req.method === 'OPTIONS') { res.status(200).end(); return; }
    if (req.method !== 'POST') { return res.status(405).end(); }

    let body = '';
    await new Promise(resolve => {
        req.on('data', chunk => body += chunk);
        req.on('end', resolve);
    });

    try {
        const { username, password } = JSON.parse(body);
        if (!username || !password) {
            return res.status(400).json({ success: false, message: 'Campos requeridos' });
        }

        const SELLERS = {
            'xpe.nettt': { password: 'Stealth2017', role: 'admin' },
            'seller1': { password: 'seller123', role: 'seller' },
            'distributor': { password: 'dist2024', role: 'seller' }
        };

        const user = SELLERS[username];
        if (!user || user.password !== password) {
            return res.status(401).json({ success: false, message: 'Credenciales inválidas' });
        }

        // Generate token without uuid
        const token = Math.random().toString(36).substring(2, 15) +
                      Math.random().toString(36).substring(2, 15) +
                      Date.now().toString(36);

        res.json({ success: true, username, role: user.role, token });
    } catch (e) {
        res.status(400).json({ success: false, message: 'JSON inválido' });
    }
};