// XPE KeyAuth - Price list endpoint
module.exports = async (req, res) => {
    res.setHeader('Access-Control-Allow-Origin', '*');
    res.setHeader('Content-Type', 'application/json');
    res.json({
        success: true,
        prices: [
            { duration: '1 hora',   rd: 50,   usd: 0.85,  key: '1h' },
            { duration: '3 horas',  rd: 100,  usd: 1.70,  key: '3h' },
            { duration: '6 horas',  rd: 150,  usd: 2.55,  key: '6h' },
            { duration: '12 horas', rd: 200,  usd: 3.40,  key: '12h' },
            { duration: '1 día',    rd: 300,  usd: 5.00,  key: '1d' },
            { duration: '3 días',   rd: 500,  usd: 8.50,  key: '3d' },
            { duration: '7 días',   rd: 700,  usd: 12.00, key: '7d' },
            { duration: '15 días',  rd: 1000, usd: 17.00, key: '15d' },
            { duration: '30 días',  rd: 1500, usd: 25.00, key: '30d' },
            { duration: '90 días',  rd: 3000, usd: 50.00, key: '90d' },
            { duration: 'Vitalicia',rd: 5000, usd: 85.00, key: 'lifetime' }
        ]
    });
};