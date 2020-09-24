// Copyright (2013) Sandia Corporation. Under the terms of Contract
// DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government 
// retains certain rights in this software.
//
// This software is released under the FreeBSD license as described 
// in License.txt

"use strict"

//
// Can a piece of code be executed? Voting has to be unanimous.
//
function VoterRegistration() {
	//
	var m_Voters = [];
	//
	function registerVoter_private(m_uniqueString, m_this, m_function) {
		m_Voters[m_uniqueString] = {"this": m_this, "function": m_function};
	}
	function unregisterVoter_private(m_uniqueString) {
		delete m_Voters[m_uniqueString];
	}
	function pollVoters_private(e) {
		var bSumVotes = true;
		for (var id in m_Voters) {
			var voter = m_Voters[id];
			var bVote = voter.function.call(voter.this, e);
			bSumVotes = bVote && bSumVotes;
		}
		return bSumVotes;
	}
	//
	this.registerVoter = function(m_uniqueString, m_this, m_function) {
		registerVoter_private(m_uniqueString, m_this, m_function);
	}
	this.unregisterVoter = function(m_uniqueString) {
		unregisterVoter_private(m_uniqueString);
	}
	this.pollVoters = function(e) {
		return pollVoters_private(e);
	}
}
