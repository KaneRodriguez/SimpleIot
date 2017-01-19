var http = require('http');
var fs = require('fs');

var firebase = require('firebase-admin');
var serviceAccount = require("./service-auth-key");
var LOGFILE = 'C:/Users/Lu_Admin/Desktop/sensorLog.txt';

firebase.initializeApp({
  credential: firebase.credential.cert(serviceAccount),
  databaseURL: 'https://wireless-sensor-nodes.firebaseio.com'
});

// Read the file and print its contents.
var fs = require('fs');


// get ref to nodes
var nodesRef = firebase.database().ref('nodes/');

// lets go ahead and delete the preexisting data before this... for now
nodesRef.remove();






/**************************************************************************************
	For the user application

	- We want the user to define conversions for each of the nodes
	- We must log each type of node that we are currently receiving
	- We will show in the UI that the user may specify a conversion (can use variables in node object)
	- We then do conversions of the data to modify the data to the users specifications before pushing to database

**************************************************************************************/
 
// object for user settings

var UserSettings = function(receivedOjectId) {
	/*
		In this object we must address:

		- Conversions from received objects to user desired objects
		- Saving of UserSettings (perhaps externally update settings for user in database?)
		- Initialize object from preexistingSettings
		- Allow conversions for attribute data
	*/
	var _this = this;
	var objectId = receivedOjectId;
	var receivedDesiredPropertyTranslation = { 
		deviceName: "deviceName",
		devicePin3Voltage: "devicePin3Voltage",
		devicePin4Voltage: "devicePin4Voltage",
		deviceTemperatureF: "deviceTemperatureF",
		deviceVoltage: "deviceVoltage",
		timeStamp: "timeStamp",
		rssiStrength: "rssiStrength"
	};
	var receivedConversionTranslation = { // TODO
		deviceName: new Function('value', 'return value;'),
		devicePin3Voltage: new Function('value', 'return value;'),
		devicePin4Voltage: new Function('value', 'return value;'),
		deviceTemperatureF: new Function('value', 'return value;'),
		deviceVoltage: new Function('value', 'return value;'),
		timeStamp: new Function('value', 'return value;'),
		rssiStrength: new Function('value', 'return value;')
	};
	this.getReceivedDesiredPropertyTranslation = function() { return receivedDesiredPropertyTranslation; };
	this.getReceivedConversionTranslation = function() { return receivedConversionTranslation; };
	this.getReceivedObjectId = function() { return objectId; };
	this.setReceivedObjectId = function(newId) { objectId = newId; };
	this.translateProperties = function(oldObj) {
		// are we allowed to configure this sensor?
		if( oldObj.deviceName == objectId) {
			// go through every propery of sensor
			for (property in oldObj) {
				if(oldObj.hasOwnProperty(property)) {
					// store the value for each property
					var propValue = oldObj[property];
					// delete the old property
					deleteFromObject(property, oldObj);

					// add the new property
					oldObj[receivedDesiredPropertyTranslation[property]] = propValue;
				}

			}
		}
	}
	this.translateConversions = function(oldObj) {
		// are we allowed to configure this sensor?
		if( oldObj.deviceName == objectId) {
			// go through every propery of sensor
			for (property in oldObj) {
				if(oldObj.hasOwnProperty(property)) {
					// call the function assigned to each property on the obj
					// the objects values will be reassigned for each property
					receivedConversionTranslation[property](oldObj);
				}

			}
		}			
	}
	this.modifyDesiredObjectStructure = function(receivedProperty, desiredProperty) {
		// do we have the receivedProperty?
		for (var property in receivedDesiredPropertyTranslation) {
			if (receivedDesiredPropertyTranslation.hasOwnProperty(property)) {
				if(property == receivedProperty) {
					// we have the property, go ahead and update the translation
					receivedDesiredPropertyTranslation[receivedProperty] = desiredProperty;
				}
			}
		}
	};
	this.modifyDesiredObjectConversion = function(receivedProperty, desiredConversionFunction) {
		// do we have the receivedProperty?
		for (var property in receivedConversionTranslation) {
			if (receivedConversionTranslation.hasOwnProperty(property)) {
				if(property == receivedProperty) {
					// we have the property, go ahead and update the translation
					receivedConversionTranslation[receivedProperty] = desiredConversionFunction;
				}
			}
		}
	};
	this.makeTranslations = function(sensor) {
		this.translateConversions(sensor);
		this.translateProperties(sensor);
	}

}
/*************************END USER APPLICATION**************************************/




// user settings for multiple objects
var settings = [];

settings[0] = new UserSettings("ED0000");
settings[1] = new UserSettings("ED0001");


// Concentration = FS* Vout / Vsupply
settings[0].modifyDesiredObjectStructure("devicePin4Voltage", "C02 (PPM)", 0);
settings[0].modifyDesiredObjectConversion("devicePin4Voltage", 
							new Function('sensor', 'var fullScale = 2000;  \
											sensor.devicePin4Voltage = fullScale * sensor.devicePin4Voltage / sensor.deviceVoltage;						\
														\
													\
										;'));

// Humidity Sensor
settings[0].modifyDesiredObjectStructure("devicePin3Voltage", "Humidity (RH)", 0);
/*
	Humidity Sensor:

	Between 20 and 90 RH

	So, to get the RH from humidity, it is a percentage of the battery voltage

	fullScale = 90;
	bottomScale = 20;

	RH Value = fullScale * AnalogOutput / batteryVoltage;

	if this RH is less than bottom scale, make it the bottom scale.
*/
settings[0].modifyDesiredObjectConversion("devicePin3Voltage", 
							new Function('sensor', 'var fullScale = 90;  \
													var bottomScale = 20; \
											sensor.devicePin3Voltage = fullScale * sensor.devicePin3Voltage / sensor.deviceVoltage;						\
											if(sensor.devicePin3Voltage < bottomScale) sensor.devicePin3Voltage = bottomScale;			\
													\
										;'));

// Concentration = FS* Vout / Vsupply
settings[1].modifyDesiredObjectStructure("devicePin3Voltage", "Light", 0);
settings[1].modifyDesiredObjectConversion("devicePin3Voltage", 
							new Function('sensor', ''));

setInterval(function() {

	processSensorData();

	// delete the logfile contents
	fs.truncate(LOGFILE, 0, function(){})

}, 250);


function processSensorData() {

	/* OBJECTIVES

		- Read directly from serial port (TODO -- use serialport library (npm install serialport --save))
		- Read log file -- done
		- Time Stamp the data -- done
		- push data to the database

	*/

	// NOTE: REIMPLEMENT THIS
	require('fs').readFileSync(LOGFILE).toString().split('\n').forEach(function (line) { 
			if(line) {
				// get sensors from read line
				var sensors = [];
				sensors = parseLineData(line);

				// update the database
				if(sensors.length) {

					var count = 0;

					for(count = 0; count < sensors.length; ++count) {
						writeSensorData(sensors[count]);					
					}
				}
			}
	 });


	// delete so the upcoming generation may not be impinged upon by the old
	sensors = [];

}


function writeSensorData(sensor) {
	var nodeHead = sensor["deviceName"];

	// TODO: Name devices better on the microcontroller. We can't have a '.' in our name
	if(nodeHead) {
		var n = nodeHead.indexOf('.');
		nodeHead = nodeHead.substring(0, n != -1 ? n : nodeHead.length);
		sensor.deviceName = nodeHead;

		if(sensor) {
			// modify this node into what the user wants
			for(var count = 0; count < settings.length; ++count) {
				settings[count].makeTranslations(sensor);
			}					
			// push to the head that has the same sensor nodeHead
			nodesRef.child(nodeHead).push(sensor);
			
		}
	}
}

function parseLineData(line) {
	var timeStamp = new Date().getTime();
	var EXPECTED_LENGTH = 39;
	var sensors = [];

	var mySensor = { // just an example, dont really use
		temperatureF: 0,
		pin3: 0,
		pin4: 0,
		id: 'XX',
		timeStamp: 0
	};
	/* EXAMPLE OF EXPECTED DATA

	 	{'deviceName':'AP','deviceVoltage':'0003.6000','deviceTemperatureF':0105.8000','devicePin3Voltage':'0000.7799','devicePin4Voltage':'0001.3800'}

		 // Look for the beginning of an object '{'
		 // look for the end of an object '}'
		 // Get all data in between
		 // create the object
		 // add object to array
		 // repeat


		 // NOTE: In the future, we will be listening using the SerialPort API. This is just so we have some data

		 // Because the conversion rate on this from received serial data to database data is bad
	*/

	// how many objects are in this line?
	if( line ) {
		var index = 0;

		for(index = 1; index < 10; ++index) { // lets process 10 shall we?
			var String = "";

			var objBeginning = getPosition(line, "{", index);
			var objEnding = getPosition(line, "}", index);
			
			var String = line.substring(objBeginning,objEnding + 1);

			// does this string really have what we want?

			if( (String[0] == "{") && (String.charAt(String.length - 1) == "}") ) {
				// was this object corrupted in transit?
				if(isJson(String)) {

					// parse json object
					var sensor = JSON.parse(String);

					// add timeStamp
					sensor.timeStamp = timeStamp;

					// add to list of sensors
					sensors.push(sensor);
				} else {
				// console.log("corrupted object: ", String);
				}
			} else {
				// console.log("didnt parse: ", String);
			}
		}
	}

	return sensors;
}



/*

	Utility Functions

*/

function checkIfNodeExists(nodeId) {
  nodesRef.child(nodeId).once('value', function(snapshot) {
    var exists = (snapshot.val() !== null);
    if(exists) {
    	return true;
    } else {
    	return false;
    }
  });
}
function getPosition(string, subString, index) {
	if(string) {
   		return string.split(subString, index).join(subString).length;
	} else {
		return -1;
	}
}
function isJson(str) {
    try {
        JSON.parse(str);
    } catch (e) {
        return false;
    }
    return true;
}
function deleteFromObject(keyPart, obj){
    for (var k in obj){          // Loop through the object
        if(~k.indexOf(keyPart)){ // If the current key contains the string we're looking for
            delete obj[k];       // Delete obj[key];
        }
    }
}

/*****************************END OF UTILITY FUNCTIONS************************/
