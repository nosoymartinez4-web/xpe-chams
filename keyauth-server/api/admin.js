// XPE KeyAuth - Admin endpoints
// POST /api/admin - List/manage licenses
// POST /api/generate - Generate new license
// POST /api/revoke - Revoke license

const { v4: uuidv4 } = require('uuid');

const licenses = global.licenses || {};
global.licenses = licenses;

// Simple token validation
function validateToken(token) {
    // For now, just check it's not empty
    return token && token.length > 0;
}

module.exports = async (req, res) => {
    res.setHeader('Access-Control-Allow-Origin', '*');
    res.setHeader('Access-Control-Allow-Methods', 'POST, GET, OPTIONS');
    res.setHeader('Access-Control-Allow-Headers', 'Content-Type');
    
    if (req.method === 'OPTIONS') {
        res.status(200).end();
        return;
    }

    const { action, token, username, role } = req.body || {};

    if (!validateToken(token)) {
        return res.status(401).json({ success: false, message: 'Unauthorized' });
    }

    switch (action) {
        case 'list':
            // Return all licenses
            const licenseList = Object.values(licenses).map(l => ({
                key: l.key,
                username: l.username,
                created: l.created,
                expiry: l.expiry,
                active: l.active,
                hwid: l.hwid || '',
                seller: l.seller || '',
                lastLogin: l.lastLogin || ''
            }));
            return res.json({ success: true, licenses: licenseList });

        case 'generate':
            // Generate new license
            const newKey = 'XPE-' + uuidv4().substring(0, 8).toUpperCase() + '-' + 
                          uuidv4().substring(0, 4).toUpperCase();
            
            const duration = parseInt(req.body.duration) || 365;
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
                seller: username || 'admin',
                lastLogin: ''
            };

            return res.json({
                success: true,
                key: newKey,
                expiry: licenses[newKey].expiry
            });

        case 'revoke':
            const revokeKey = req.body.key;
            if (!revokeKey || !licenses[revokeKey]) {
                return res.json({ success: false, message: 'License not found' });
            }
            licenses[revokeKey].active = false;
            return res.json({ success: true, message: 'License revoked' });

        case 'reactivate':
            const reactKey = req.body.key;
            if (!reactKey || !licenses[reactKey]) {
                return res.json({ success: false, message: 'License not found' });
            }
            licenses[reactKey].active = true;
            return res.json({ success: true, message: 'License reactivated' });

        case 'delete':
            const delKey = req.body.key;
            if (!delKey || !licenses[delKey]) {
                return res.json({ success: false, message: 'License not found' });
            }
            delete licenses[delKey];
            return res.json({ success: true, message: 'License deleted' });

        case 'stats':
            const total = Object.keys(licenses).length;
            const active = Object.values(licenses).filter(l => l.active).length;
            const expired = Object.values(licenses).filter(l => {
                return new Date(l.expiry) < new Date();
            }).length;
            return res.json({
                success: true,
                stats: { total, active, expired, revoked: total - active }
            });

        default:
            return res.json({ success: false, message: 'Unknown action' });
    }
};