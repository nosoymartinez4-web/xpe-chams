// XPE KeyAuth - Seller endpoints
// POST /api/seller - Generate & manage licenses for sellers

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

    const { action, token, username } = req.body || {};

    // Simple auth check
    if (!token || token.length < 5) {
        return res.status(401).json({ success: false, message: 'Unauthorized' });
    }

    switch (action) {
        case 'generate':
            const duration = parseInt(req.body.duration) || 30;
            const newKey = 'XPE-' + uuidv4().substring(0, 8).toUpperCase() + '-' + 
                          uuidv4().substring(0, 4).toUpperCase();
            
            const created = new Date();
            const expiry = new Date(created);
            expiry.setDate(expiry.getDate() + duration);

            licenses[newKey] = {
                key: newKey,
                username: req.body.customer || '',
                created: created.toISOString().split('T')[0],
                expiry: expiry.toISOString().split('T')[0],
                active: true,
                hwid: '',
                seller: username || 'unknown',
                lastLogin: ''
            };

            return res.json({
                success: true,
                key: newKey,
                expiry: licenses[newKey].expiry
            });

        case 'list':
            const myLicenses = Object.values(licenses).filter(l => l.seller === username);
            return res.json({
                success: true,
                licenses: myLicenses.map(l => ({
                    key: l.key,
                    username: l.username,
                    created: l.created,
                    expiry: l.expiry,
                    active: l.active,
                    hwid: l.hwid || '',
                    lastLogin: l.lastLogin || ''
                }))
            });

        case 'stats':
            const sellerLicenses = Object.values(licenses).filter(l => l.seller === username);
            const total = sellerLicenses.length;
            const active = sellerLicenses.filter(l => l.active).length;
            return res.json({
                success: true,
                stats: { total, active, revoked: total - active }
            });

        default:
            return res.json({ success: false, message: 'Unknown action' });
    }
};