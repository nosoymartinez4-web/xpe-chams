// XPE KeyAuth - Auth endpoint
module.exports = async (req, res) => {
    res.setHeader('Access-Control-Allow-Origin', '*');
    res.setHeader('Access-Control-Allow-Methods', 'POST, OPTIONS');
    res.setHeader('Access-Control-Allow-Headers', 'Content-Type');
    if (req.method === 'OPTIONS') { res.status(200).end(); return; }
    if (req.method !== 'POST') { return res.status(405).json({ success: false }); }

    // Vercel already parses JSON body into req.body
    // If not, try manual
    let data;
    if (req.body && Object.keys(req.body).length > 0) {
        data = req.body;
    } else {
        let body = '';
        await new Promise(resolve => { req.on('data', c => body += c); req.on('end', resolve); });
        try { data = JSON.parse(body); } catch(e) { return res.status(400).json({ success: false, message: 'JSON inválido' }); }
    }

    const { username, password } = data;
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

    const token = Math.random().toString(36).substring(2, 15) +
                  Math.random().toString(36).substring(2, 15) +
                  Date.now().toString(36);

    res.json({ success: true, username, role: user.role, token });
};