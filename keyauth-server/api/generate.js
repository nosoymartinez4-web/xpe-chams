// XPE KeyAuth - Generate license (alias endpoint)

const { v4: uuidv4 } = require('uuid');

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

    const { token, duration, customer, seller } = req.body || {};

    if (!token || token.length < 5) {
        return res.status(401).json({ success: false, message: 'Unauthorized' });
    }

    const days = parseInt(duration) || 365;
    const newKey = 'XPE-' + uuidv4().substring(0, 8).toUpperCase() + '-' + 
                  uuidv4().substring(0, 4).toUpperCase();
    
    const created = new Date();
    const expiry = new Date(created);
    expiry.setDate(expiry.getDate() + days);

    licenses[newKey] = {
        key: newKey,
        username: customer || '',
        created: created.toISOString().split('T')[0],
        expiry: expiry.toISOString().split('T')[0],
        active: true,
        hwid: '',
        seller: seller || 'admin',
        lastLogin: ''
    };

    res.json({
        success: true,
        key: newKey,
        expiry: licenses[newKey].expiry
    });
};