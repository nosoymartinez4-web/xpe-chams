// XPE KeyAuth - Revoke license endpoint

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

    const { token, key } = req.body || {};

    if (!token || token.length < 5) {
        return res.status(401).json({ success: false, message: 'Unauthorized' });
    }

    if (!key || !licenses[key]) {
        return res.json({ success: false, message: 'License not found' });
    }

    licenses[key].active = false;

    res.json({
        success: true,
        message: 'License revoked successfully'
    });
};