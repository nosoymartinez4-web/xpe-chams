// XPE KeyAuth - Auth endpoint
// Admin login & seller login

const { v4: uuidv4 } = require('uuid');

module.exports = async (req, res) => {
    // CORS
    res.setHeader('Access-Control-Allow-Origin', '*');
    res.setHeader('Access-Control-Allow-Methods', 'POST, OPTIONS');
    res.setHeader('Access-Control-Allow-Headers', 'Content-Type');
    
    if (req.method === 'OPTIONS') {
        res.status(200).end();
        return;
    }
    
    if (req.method !== 'POST') {
        return res.status(405).json({ success: false, message: 'Method not allowed' });
    }

    const { username, password } = req.body || {};

    if (!username || !password) {
        return res.status(400).json({ success: false, message: 'Username and password required' });
    }

    // Admin credentials
    const ADMIN_USER = 'xpe.nettt';
    const ADMIN_PASS = 'Stealth2017';

    // Seller credentials (hardcoded for now)
    const SELLERS = {
        'xpe.nettt': { password: 'Stealth2017', role: 'admin' },
        'seller1': { password: 'seller123', role: 'seller' },
        'distributor': { password: 'dist2024', role: 'seller' }
    };

    const user = SELLERS[username];
    if (!user || user.password !== password) {
        return res.status(401).json({ success: false, message: 'Invalid credentials' });
    }

    // Generate session token
    const token = uuidv4();
    const sessionData = {
        username,
        role: user.role,
        token,
        loginTime: new Date().toISOString()
    };

    // In production, store session in memory or DB
    // For now, return token to client

    res.json({
        success: true,
        username,
        role: user.role,
        token
    });
};