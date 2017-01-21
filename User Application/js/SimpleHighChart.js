var SimpleHighChart = function(options) {
	var _this = this;
	this.options = options;
	this.myHighChart;
	this.myChart;
	this.myPoint;
	this.makeChart = function() {
		var options = this.options;
		var chartTitle = options.chartTitle;
		var chartId = options.chartId;
		var minimumChartValue = options.minimumChartValue;
		var maximumChartValue = options.maximumChartValue;
		var comfortLevelMax = options.comfortLevelMax;
		var comfortLevelMin = options.comfortLevelMin;
		var belowComfortColor = options.belowComfortColor;
		var aboveComfortColor = options.aboveComfortColor;
		var comfortColor = options.comfortColor;
		var chartStartAngle = options.chartStartAngle;
		var chartStopAngle = options.chartStopAngle;
		var chartUnits = options.chartUnits;

		var chartRange = maximumChartValue - minimumChartValue;
		var comfortRange = comfortLevelMax - comfortLevelMin;
		var belowComfortRange = (comfortLevelMin);
		var aboveComfortRange = (chartRange - comfortLevelMax);
		var belowComfortStart = minimumChartValue;
		var belowComfortEnd = minimumChartValue + belowComfortRange;
		var comfortStart = belowComfortEnd;
		var comfortEnd = belowComfortEnd + comfortRange;
		var aboveComfortStart = comfortEnd;
		var aboveComfortEnd = comfortEnd + aboveComfortRange;

		
		
		$(function () {

			_this.myHighChart = Highcharts.chart(chartId, {

				chart: {
					type: 'gauge',
					plotBackgroundColor: null,
					plotBackgroundImage: null,
					plotBorderWidth: 0,
					plotShadow: false
				},

				title: {
					text: chartTitle
				},

				pane: {
					startAngle: chartStartAngle,
					endAngle: chartStopAngle,
					background: [{
						backgroundColor: {
							linearGradient: { x1: 0, y1: 0, x2: 0, y2: 1 },
							stops: [
								[0, '#FFF'],
								[1, '#333']
							]
						},
						borderWidth: 0,
						outerRadius: '109%'
					}, {
						backgroundColor: {
							linearGradient: { x1: 0, y1: 0, x2: 0, y2: 1 },
							stops: [
								[0, '#333'],
								[1, '#FFF']
							]
						},
						borderWidth: 1,
						outerRadius: '107%'
					}, {
						// default background
					}, {
						backgroundColor: '#DDD',
						borderWidth: 0,
						outerRadius: '105%',
						innerRadius: '103%'
					}]
				},

				// the value axis
				yAxis: {
					min: minimumChartValue,
					max: maximumChartValue,

					minorTickInterval: 'auto',
					minorTickWidth: 1,
					minorTickLength: 10,
					minorTickPosition: 'inside',
					minorTickColor: '#666',

					tickPixelInterval: 30,
					tickWidth: 2,
					tickPosition: 'inside',
					tickLength: 10,
					tickColor: '#666',
					labels: {
						step: 2,
						rotation: 'auto'
					},
					title: {
						text: chartUnits
					},
					plotBands: [{
						from: belowComfortStart,
						to: belowComfortEnd,
						color: belowComfortColor // green
					}, {
						from: comfortStart,
						to: comfortEnd,
						color: comfortColor // yellow
					}, {
						from: aboveComfortStart,
						to: aboveComfortEnd,
						color: aboveComfortColor // red
					}]
				},

				series: [{
					name: chartTitle,
					data: [0],
					tooltip: {
						valueSuffix: ' ' + chartUnits
					}
				}]

			},
			function (chart) {
				if (!chart.renderer.forExport) {
					_this.myChart = chart;
					_this.myPoint = chart.series[0].points[0];
				}
			});
		});

		setTimeout(function() { 
			var peskyCredits = $(".highcharts-credits");
			var peskyButton = $(".highcharts-button-symbol");
			peskyCredits.hide();
			peskyButton.hide();
			peskyButton.parent().hide();
		}, 0.00000000000001);


	}
	this.updatePoint = function(newValue) {
		if(newValue) {
			newValue = parseFloat(newValue).toFixed(2);
			if(newValue > _this.options.maximumChartValue) {
				newValue = _this.options.maximumChartValue;
			}
			_this.myPoint.update(parseFloat(newValue));
		}		
	}
	this.getComfortability = function() {
		// are we above, below, or in the comfortability range?
		var response = "...";
		if(_this.myPoint.y > _this.options.comfortLevelMax) { // is it above the comfort threshold?
			response = options.aboveComfortResponse;
		} else if(_this.myPoint.y < _this.options.comfortLevelMin) { // is it below the comfort threshold?
			response = options.belowComfortResponse;
		} else {
			response = options.comfortResponse;
		}
		
		return response;
	};
	this.getCurrentComfortColor = function() {
		var color = "grey";
		if(_this.myPoint.y > _this.options.comfortLevelMax) { // is it above the comfort threshold?
			color = _this.options.aboveComfortColor;
		} else if(_this.myPoint.y < _this.options.comfortLevelMin) { // is it below the comfort threshold?
			color = _this.options.belowComfortColor;
		} else {
			color = _this.options.comfortColor;
		}			
		return color;
	}
}
// move this to its own file eventually
var MeterReading = function(selectedTitle, selectedUnits) {
	var _this = this;
	this.itsTitle = selectedTitle;
	this.itsUnits = selectedUnits;

	this.html = "<li></li>";
	this.makeReading = function(title, units) {
		
		var type = '<td id="' + title + 'Title">' + title + '</td>';
		var units = '<td id="' + title + 'Units">' + units + '</td>';
		var comfort = '<td id="' + title + 'Comfort">Comfort Level </td>';
		var reading = '<td id="' + title + 'Reading">0</td> ';
		var lastUpdated = '<td id="' + title + 'LastUpdated">Long Ago..</td> ';
		
		return '<tr id="' + title + '">' + type + comfort + reading + units + lastUpdated + ' </tr>';
	}	
	this.updateReading = function(reading, comfort, comfortColor, time) {
		_this.itsComfort = comfort;
		_this.itsReading = reading;
		$("#" + _this.itsTitle + "Comfort").text(comfort);
		$("#" + _this.itsTitle + "Reading").text(reading);
		$("#" + _this.itsTitle + "LastUpdated").text(time);
		
		// update the table row color
		$("#" + _this.itsTitle).css({background: comfortColor, color: "white"});
	}
	var init = function() {
		_this.html = _this.makeReading(_this.itsTitle, _this.itsUnits);
	}
	init();
}