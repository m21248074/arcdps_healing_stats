## ArcDPS Healing Stats
Show healing statistics based on your local stats (i.e. your own healing output).

This includes outgoing healing per agent and per skill, as well as filtering to only include your own subgroup/squad or to exclude minions

# Usage
Toggle "Heal Table" in ArcDPS options

The **Sort Order** option changes how agents and skills are sorted under the *Agents* and *Skills* headers. Currently the two supported options are by name alphabetically and by amount of healing. Both sort options can be done ascending or descending.

The **Group Filter** option filters which agents should be displayed. This also filters skills accordingly to only show healing applied to the filtered agents. The possible options are:
- <ins>Group</ins>: Agents in the same subgroup as yourself
- <ins>Squad</ins>: Agents in your squad
- <ins>All (Excluding minions)</ins>: All healing, not counting minions (minions in this case means all player spawned agents, such as pets, spirits, clones etc.)
- <ins>All (Including minions)</ins>: All healing

The **Exclude unmapped agents** option will, when checked, filter out agents whose name is unknown. When not checked, you might see agent names which are just numbers (indicating that the agent is not mapped). Since their subgroup is unknown, these agents will be attributed to all group filters. This option will therefore also affect all the totals shown under the *Totals* header.

# Installation
Requires [ArcDPS](https://www.deltaconnected.com/arcdps/).

Drag and drop arcdps_healing_stats.dll into the bin64\ directory (same directory as where arcdps d3d9.dll is)

# Issues and requests
Please report issues and requests using the github issue tracker

# Pictures
![Example](./Example.png)

# Technical information
This addon uses the local stats provided by ArcDPS to count healing done. This information is only available for the local player, i.e. the server does not notify about healing done by other players to other players. As such it is not possible to extend the addon to include everyone's healing without every player in the instance having the addon installed.

# Planned features
- Display healing done per pulse of the skill (which indirectly corresponds to healing per skill activation)
- Breakdown per skill and per agent similar to that available in the vanilla ArcDPS dps window (i.e. clicking a skill would bring up a new window showing which agents were healed for how much with that skill, clicking an agent would bring up a new window showing which skills healed that agent for how much)
- Store history similar to that available in the vanilla ArcDPS dps window (i.e. show statistics for previous encounters)
- Generate a healing log, similar to the .evtc one generated by ArcDPS. This also requires a program similar to Elite Insights that can process such logs. That program could also potentially combine 10 logs from the squad to generate a full log that shows all healing done by the whole squad.
- Display a graph showing healing done over time (allowing visualisation of when healing pressure is high)
- More statistics than just healing. Confusion comes to mind, since the 10 man log method mentioned above could in the case of confusion create a log that shows true dps done for bosses where self stats and area stats don't match (such as Soulless Horror)