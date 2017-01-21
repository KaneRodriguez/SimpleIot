// Initialize Firebase
  var config = {
    apiKey: "AIzaSyDqdi-VHqkYVC3lcysnLkh9lOEuLf0_arg",
    authDomain: "wireless-sensor-nodes.firebaseapp.com",
    databaseURL: "https://wireless-sensor-nodes.firebaseio.com",
    storageBucket: "wireless-sensor-nodes.appspot.com",
    messagingSenderId: "1088652518127"
  };
  firebase.initializeApp(config);
	
var dbRef = new Firebase("https://wireless-sensor-nodes.firebaseio.com/");
var nodesRef = dbRef.child('nodes')
var initialDataLoaded = false;

var humidityOptions = {
	chartTitle: "Humidity",
	chartId: 'humidityChart',
	minimumChartValue: 0,
	maximumChartValue: 100,
	comfortLevelMax: 65,
	comfortLevelMin: 35,
	belowComfortColor: "blue",
	aboveComfortColor: "red",
	comfortColor: "green",
	chartStartAngle: -100,
	chartStopAngle: 100,
	chartUnits: "RH",
	aboveComfortResponse: "Too Wet",
	belowComfortResponse: "Too Dry",
	comfortResponse: "Just Right"
};
var temperatureOptions = {
	chartTitle: "Temperature",
	chartId: 'temperatureChart',
	minimumChartValue: 0,
	maximumChartValue: 100,
	comfortLevelMax: 76,
	comfortLevelMin: 64,
	belowComfortColor: "blue",
	aboveComfortColor: "red",
	comfortColor: "green",
	chartStartAngle: -100,
	chartStopAngle: 100,
	chartUnits: "F",
	aboveComfortResponse: "Too Hot",
	belowComfortResponse: "Too Cold",
	comfortResponse: "Just Right"
};
var co2Options = {
	chartTitle: "CO2",
	chartId: 'co2Chart',
	minimumChartValue: 0,
	maximumChartValue: 2000,
	comfortLevelMax: 1500,
	comfortLevelMin: 800,
	belowComfortColor: "blue",
	aboveComfortColor: "red",
	comfortColor: "green",
	chartStartAngle: -100,
	chartStopAngle: 100,
	chartUnits: "PPM",
	aboveComfortResponse: "Too Toxic",
	belowComfortResponse: "Too little CO2 (still ok)",
	comfortResponse: "Just Right"
};

var humidityChart = new SimpleHighChart(humidityOptions);
var temperatureChart = new SimpleHighChart(temperatureOptions);
var co2Chart = new SimpleHighChart(co2Options);

var temperatureChartDOM = $("#temperatureChart");
var humidityChartDOM = $("#humidityChart");
var co2ChartDOM = $("#co2Chart");
	
var meters = $(".meter"); // name confusion, these are the charts

var meterReadings = $(".meterReadings");
var tempMeter = new MeterReading("Temperature", "F");
var humidityMeter = new MeterReading("Humidity", "RH");
var co2Meter = new MeterReading("Carbon-Dioxide", "PPM");

// create all of the meters (text representation of charts)
meterReadings.children("tbody").append(tempMeter.html, humidityMeter.html, co2Meter.html);

humidityChart.makeChart();
temperatureChart.makeChart();
co2Chart.makeChart();

initializeData();

nodesRef.on("child_added", function(snapshot) {
	// ?
});
function updateSimpleHighCharts(object) {
	var co2Level = object["C02 (PPM)"];
	var humidityLevel = object["Humidity (RH)"];
	var temperatureLevel = object["deviceTemperatureF"]; // subject to change the name
	var timeStamp = new Date(object.timeStamp);
	var time = timeStamp.getHours() % 12 + ":" + timeStamp.getMinutes() + ":" + timeStamp.getSeconds() + ( timeStamp.getHours() < 12 ? " am": " pm" );
	
	if(object["deviceName"] != "AP") { // the AP has a warmer temp due to its heavy amount of processing
		
		updateSimpleHighChart(temperatureChart, temperatureLevel, tempMeter, temperatureChartDOM, time);
		
	}
	updateSimpleHighChart(co2Chart, co2Level, co2Meter, co2ChartDOM, time);
	updateSimpleHighChart(humidityChart, humidityLevel, humidityMeter, humidityChartDOM, time);
}
function updateSimpleHighChart(chart, newValue, meter, chartDOM, time) {
	chart.updatePoint(newValue);
	meter.updateReading(chart.myPoint.y, chart.getComfortability(), chart.getCurrentComfortColor(), time);
	chartDOM.css({border: '.3em solid ' + chart.getCurrentComfortColor()});	
}

function initializeData() {

	nodesRef.once('value', function(snapshot) {
	// for each of the nodes for this system
		snapshot.forEach(function(childSnapshot) {
			// get the most recent sensor value for each node
			nodesRef.child(childSnapshot.key()).limitToLast(1).once('value', function(fbObject) {
				// do some acrobatics to get the object we want...
				var parentObject = fbObject.val();
				var object = parentObject[Object.keys(parentObject)[0]]; //returns value of first property (the object)
				
				// use our object to update the charts
				updateSimpleHighCharts(object);
			});		
			
			// now lets listen for new additional values from these children
			listenForSensorValues(nodesRef.child(childSnapshot.key()));
		});
		listenForNewChildren(nodesRef);
		// end of once
		initialDataLoaded = true;
	});
}
function listenForNewChildren(ref) {
	ref.on("child_added", function(snapshot) {
			if(initialDataLoaded) {
				console.log(snapshot.val()); // TODO
			}
	});
}
function listenForSensorValues(ref) {
		ref.on("child_added", function(snapshot) {
			if(initialDataLoaded) {
				updateSimpleHighCharts(snapshot.val());
			}
		});
}



