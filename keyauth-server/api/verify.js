// XPE KeyAuth - Verify license key (called by the DLL)
// POST /api/verify

// In-memory license storage (in production use Vercel KV or a JSON file)
const licenses = global.licenses || {};
global.licenses = licenses;

// Seed some demo licenses if empty
if (Object.keys(licenses).length === 0) {
    licenses['XPE-DEMO-2024-ABCD'] = {
        key: 'XPE-DEMO-2024-ABCD',
        username: 'demo_user',
        created: '2024-01-15',
        expiry: '2025-01-15',
        active: true,
        hwid: '',
        seller: 'xpe.nettt'
    };
    licenses['XPE-TEST-1234'] = {
        key: 'XPE-TEST-1234',
        username: 'tester',
        created: '2024-06-01',
        expiry: '2025-06-01',
        active: true,
        hwid: '',
        seller: 'xpe.nettt'
    };
}

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

    const { key, hwid } = req.body || {};

    if (!key) {
        return res.status(400).json({ success: false, message: 'License key required' });
    }

    const license = licenses[key];
    if (!license) {
        return res.json({ success: false, message: 'Invalid license key' });
    }

    if (!license.active) {
        return res.json({ success: false, message: 'License has been revoked' });
    }

    // Check expiry
    const now = new Date();
    const expiry = new Date(license.expiry);
    if (now > expiry) {
        license.active = false;
        return res.json({ success: false, message: 'License has expired' });
    }

    // Bind HWID on first use
    if (!license.hwid && hwid) {
        license.hwid = hwid;
    }

    // Check HWID match
    if (license.hwid && hwid && license.hwid !== hwid) {
        return res.json({ success: false, message: 'License already in use on another PC' });
    }

    // Update last login
    license.lastLogin = now.toISOString();

    res.json({
        success: true,
        username: license.username || key,
        expiry: license.expiry,
        key: license.key
    });
};