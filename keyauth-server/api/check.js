// XPE KeyAuth - Check license status (called periodically by DLL)
// POST /api/check

const licenses = global.licenses || {};
global.licenses = licenses;

module.exports = async (req, res) => {
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

    const { key } = req.body || {};

    if (!key) {
        return res.status(400).json({ success: false, message: 'License key required' });
    }

    const license = licenses[key];
    if (!license) {
        return res.json({ success: false, message: 'Invalid license key' });
    }

    if (!license.active) {
        return res.json({ success: false, message: 'License revoked' });
    }

    // Check expiry
    const now = new Date();
    const expiry = new Date(license.expiry);
    if (now > expiry) {
        license.active = false;
        return res.json({ success: false, message: 'License expired' });
    }

    res.json({
        success: true,
        username: license.username || key,
        expiry: license.expiry
    });
};