const Alexa = require('alexa-bootstrap')
const Express = require('express')
const BodyParser = require('body-parser')

const port = process.env.PORT || 5000
const alexa = new Alexa({
    repeat: true,
    applicationId: 'skill_id',
    shouldEndSession: false

})
const router = Express.Router()
const express = Express()


express.set('port', port)

express.use(BodyParser.urlencoded({extended: true}))
express.use(BodyParser.json())




alexa.sessionEnded((req, res) => {
    res.say("buy buy")
    console.log('SessionEnded')
})

router.post('/skill', (req, res) => {
    alexa.request(req.body).then(response => {
        res.json(response)
    }, response => {
        res.status(500).send('Server Error')
    })
})

express.use('/', router)
express.listen(port, () => {
    console.log(`Alexa app is running on port ${port}`)
})

var net = require('net');
var sqlite3 = require('sqlite3').verbose();
var db = new sqlite3.Database('cvents.db');

db.serialize(function () {
    console.log("create table");
    db.run("CREATE TABLE IF NOT EXISTS cvents (MQ7 INTEGER, rpm INTEGER);");
});
var stmt = db.prepare("INSERT INTO cvents (MQ7, rpm) VALUES(?, ?)");

var s;
var server = net.createServer(function (socket) {
    console.log('server connected');

    socket.on('end', function () {
        console.log('server disconected');
    });
    socket.on('data', function (data) {
        var str = data.toString('utf8');
        var d = str.split("\t");
        stmt.run(d[0], d[2]);
        console.log(data.toString('utf8'));
    });
    socket.write('Clever Vents\r\n');

    alexa.intent('SET', (req, res) => {
        var val = req.slot('VALUE');
        if (val <= 100 && val >= 0)
            res.say('Ok, set to {value} percent', {value: val})
        else res.say('In percent please')
        socket.write('\t1\t' + val);
    });

    alexa.intent('MAX', (req, res) => {
        socket.write('\t1\t100');
        res.say('Full power')
                .reprompt('I cant hear you. Say yes or no.')
    });
    alexa.intent('MIN', (req, res) => {
        socket.write('\t1\t0');
        res.say('Minimal value')
                .reprompt('I cant hear you. Say yes or no.')
    });
    alexa.intent('RISE', (req, res) => {
        socket.write('\t2\t10');
        res.say('Rise')
                .reprompt('I cant hear you. Say yes or no.')
    });
    alexa.intent('REDUCE', (req, res) => {
            socket.write('\t2\t-1');
        res.say('Spee will slowly reduced')
                .reprompt('I cant hear you. Say yes or no.')
    });
    
});

server.listen(8080, 'localhost');
